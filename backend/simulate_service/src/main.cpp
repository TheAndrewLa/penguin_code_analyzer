#include <drogon/drogon.h>

#include "x86_simulation.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
constexpr auto DefaultPort = std::uint16_t(8080);

constexpr auto MaxBodySize = 64U * 1024U * 1024U;

struct Params {
  std::string cpu;
  std::vector<std::string> instructions;
};

drogon::HttpResponsePtr MakeJsonError(const std::string &message, drogon::HttpStatusCode code) {
  Json::Value body;
  body["error"] = message;

  auto response = drogon::HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(code);

  return response;
}

llvm::Expected<Json::Value> GetJsonBody(const drogon::HttpRequestPtr &req) {
  const auto body = Json::String(req->getBody());

  if (body.empty()) {
    return llvm::createStringError("Request body must not be empty!");
  }

  auto stream = Json::IStringStream(body);

  Json::CharReaderBuilder builder;
  Json::Value root;
  Json::String errors;

  if (Json::parseFromStream(builder, stream, &root, &errors)) {
    return root;
  }

  return llvm::createStringError("Can not parse JSON object!");
}

llvm::Expected<Params> ExtractSimulationParams(const Json::Value &object) {
  std::string cpu;
  std::vector<std::string> instructions;

  if (object.isMember("cpu") && object["cpu"].isString()) {
    cpu = object["cpu"].asString();
  } else {
    return llvm::createStringError("Field 'cpu' is not set!");
  }

  if (!object.isMember("instructions") || !object["instructions"].isArray()) {
    return llvm::createStringError("Field 'instructions' must be an array of strings!");
  }

  for (const auto &instr : object["instructions"]) {
    if (!instr.isString()) {
      return llvm::createStringError("Instruction must be a string!");
    }
    instructions.emplace_back(instr.asString());
  }

  return Params{std::move(cpu), std::move(instructions)};
}

using SessionGetter = X86Simulation::JsonResult (X86Simulation::*)(const std::string &);

void RegisterResultsHandler(drogon::HttpAppFramework &app, const std::string &path, X86Simulation &service,
                            SessionGetter method) {
  app.registerHandler(path,
                      [&service, method](const drogon::HttpRequestPtr &req,
                                         std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                        const auto session = req->getParameter("session");

                        if (session.empty()) {
                          callback(MakeJsonError("Query parameter 'session' is not set!", drogon::k400BadRequest));
                          return;
                        }

                        auto result = (service.*method)(session);

                        if (!result) {
                          callback(MakeJsonError(llvm::toString(result.takeError()), drogon::k400BadRequest));
                        } else {
                          callback(drogon::HttpResponse::newHttpJsonResponse(*result));
                        }
                      },
                      {drogon::Get});
}
} // namespace

int main(int argc, char *argv[]) {
  auto port = DefaultPort;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
      port = std::stoi(argv[++i]);
    }
  }

  auto service = X86Simulation();
  auto &app = drogon::app();

  app.addListener("0.0.0.0", port);
  app.setThreadNum(std::thread::hardware_concurrency());

  app.setClientMaxBodySize(MaxBodySize);
  app.setClientMaxMemoryBodySize(MaxBodySize);

  app.registerHandler(
      "/health",
      [](const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        auto body = Json::Value(Json::objectValue);
        body["status"] = "ok";

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
      },
      {drogon::Get});

  app.registerHandler(
      "/start",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        auto json = GetJsonBody(req);

        if (!json) {
          callback(MakeJsonError(llvm::toString(json.takeError()), drogon::k400BadRequest));
          return;
        }

        auto params = ExtractSimulationParams(*json);

        if (!params) {
          callback(MakeJsonError(llvm::toString(params.takeError()), drogon::k400BadRequest));
          return;
        }

        const auto session = service.start(params->cpu, params->instructions);

        auto body = Json::Value(Json::objectValue);
        body["session"] = session;

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
      },
      {drogon::Post});

  app.registerHandler(
      "/end",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        const auto &session = req->getParameter("session");

        if (session.empty()) {
          callback(MakeJsonError("Query parameter 'session' is not set!", drogon::k400BadRequest));
          return;
        }

        service.end(session);

        auto body = Json::Value(Json::objectValue);
        body["status"] = "ok";

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
      },
      {drogon::Get});

  RegisterResultsHandler(app, "/getGeneral", service, &X86Simulation::getGeneralResults);
  RegisterResultsHandler(app, "/getInstructions", service, &X86Simulation::getInstructionInfo);
  RegisterResultsHandler(app, "/getResources", service, &X86Simulation::getResourceUsage);
  RegisterResultsHandler(app, "/getTimeline", service, &X86Simulation::getTimeline);

  std::cout << "x86 simulation service listening on port " << port << std::endl;

  app.run();

  return 0;
}
