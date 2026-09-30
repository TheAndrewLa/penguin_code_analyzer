#include <drogon/drogon.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "x86_instr_info.hpp"

namespace {
constexpr auto DefaultPort = std::uint16_t(8080);

constexpr auto MaxBodySize = 64U * 1024U * 1024U;

struct Params {
  Json::String cpu;
  Json::String instruction;
};

drogon::HttpResponsePtr MakeJsonError(const std::string &message, drogon::HttpStatusCode code) {
  Json::Value body;
  body["error"] = message;

  auto response = drogon::HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(code);

  return response;
}

llvm::Expected<Json::Value> GetJsonBody(const drogon::HttpRequestPtr &req) {
  auto json = req->getJsonObject();

  if (json) {
    return *json;
  }

  const auto body = Json::String(req->getBody());
  auto stream = Json::IStringStream(body);

  Json::CharReaderBuilder builder;
  Json::Value root;
  Json::String errors;

  if (Json::parseFromStream(builder, stream, &root, &errors)) {
    return root;
  }

  return llvm::createStringError("Can not parse JSON object!");
}

llvm::Expected<Params> ExtractParams(const Json::Value &object) {
  std::string cpu;
  std::string instruction;

  if (object.isMember("cpu") && object["cpu"].isString()) {
    cpu = object["cpu"].asString();
  } else {
    return llvm::createStringError("Field 'cpu' is not set!");
  }

  if (object.isMember("instruction") && object["instruction"].isString()) {
    instruction = object["instruction"].asString();
  } else {
    return llvm::createStringError("Microarchitecture name must not be empty");
  }

  return Params{cpu, instruction};
}
} // namespace

int main(int argc, char **argv) {
  auto port = DefaultPort;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
      port = std::stoi(argv[++i]);
    }
  }

  auto service = X86InstrInfo{};
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
      "/brief",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        auto json = GetJsonBody(req);

        if (!json) {
          callback(MakeJsonError(llvm::toString(json.takeError()), drogon::k400BadRequest));
          return;
        }

        auto analyzeReq = ExtractParams(*json);

        if (!analyzeReq) {
          callback(MakeJsonError(llvm::toString(analyzeReq.takeError()), drogon::k400BadRequest));
          return;
        }

        auto briefResult = service.brief(analyzeReq->cpu, analyzeReq->instruction);

        if (!briefResult) {
          callback(MakeJsonError(llvm::toString(briefResult.takeError()), drogon::k400BadRequest));
        } else {
          callback(drogon::HttpResponse::newHttpJsonResponse(*briefResult));
        }
      },
      {drogon::Post});

  app.registerHandler(
      "/full",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        auto json = GetJsonBody(req);

        if (!json) {
          callback(MakeJsonError(llvm::toString(json.takeError()), drogon::k400BadRequest));
          return;
        }

        auto reqBody = ExtractParams(*json);

        if (!reqBody) {
          callback(MakeJsonError(llvm::toString(reqBody.takeError()), drogon::k400BadRequest));
          return;
        }

        auto fullResult = service.full(reqBody->cpu, reqBody->instruction);

        if (!fullResult) {
          callback(MakeJsonError(llvm::toString(fullResult.takeError()), drogon::k400BadRequest));
        } else {
          callback(drogon::HttpResponse::newHttpJsonResponse(*fullResult));
        }
      },
      {drogon::Post});

  std::cout << "x86 instruction service listening on port " << port << std::endl;

  app.run();

  return 0;
}
