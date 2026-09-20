#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "x86_instr_info.hpp"

#ifndef PORT
#define PORT (8888)
#endif

namespace {
drogon::HttpResponsePtr MakeJsonError(const std::string &message, drogon::HttpStatusCode code = drogon::k200OK) {
  Json::Value body;
  body["error"] = message;

  auto response = drogon::HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(code);

  return response;
}

std::shared_ptr<Json::Value> GetJsonBody(const drogon::HttpRequestPtr &req) {
  auto json = req->getJsonObject();

  if (json) {
    return json;
  }

  Json::CharReaderBuilder builder;
  Json::Value parsed;
  std::string errors;

  const auto body = std::string{req->getBody()};
  std::istringstream stream(body);

  if (Json::parseFromStream(builder, stream, &parsed, &errors)) {
    return std::make_shared<Json::Value>(parsed);
  }

  return nullptr;
}

llvm::Expected<std::pair<std::string, std::string>> ExtractReqBody(const Json::Value &json) {
  std::string cpu;
  std::string instruction;

  if (json.isMember("cpu") && json["cpu"].isString()) {
    cpu = json["cpu"].asString();
  } else {
    return llvm::createStringError("Field 'cpu' is not set!");
  }

  if (json.isMember("instruction") && json["instruction"].isString()) {
    instruction = json["instruction"].asString();
  } else {
    return llvm::createStringError("Microarchitecture name must not be empty");
  }

  return std::pair<std::string, std::string>{cpu, instruction};
}
} // namespace

int main(int argc, char **argv) {
  const auto port = std::uint16_t(PORT);

  auto service = X86InstrInfo{};
  auto &app = drogon::app();

  app.addListener("0.0.0.0", port);
  app.setThreadNum(std::thread::hardware_concurrency());

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
          callback(MakeJsonError("JSON request body required", drogon::k400BadRequest));
          return;
        }

        auto analyzeReq = ExtractReqBody(*json);

        if (!analyzeReq) {
          callback(MakeJsonError(llvm::toString(analyzeReq.takeError()), drogon::k400BadRequest));
          return;
        }

        auto briefResult = service.brief(analyzeReq->first, analyzeReq->second);

        if (!briefResult) {
          callback(MakeJsonError(llvm::toString(briefResult.takeError())));
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
          callback(MakeJsonError("JSON request body required", drogon::k400BadRequest));
          return;
        }

        auto reqBody = ExtractReqBody(*json);

        if (!reqBody) {
          callback(MakeJsonError(llvm::toString(reqBody.takeError()), drogon::k400BadRequest));
          return;
        }

        auto fullResult = service.brief(reqBody->first, reqBody->second);

        if (!fullResult) {
          callback(MakeJsonError(llvm::toString(fullResult.takeError())));
        } else {
          callback(drogon::HttpResponse::newHttpJsonResponse(*fullResult));
        }
      },
      {drogon::Post});

  std::cout << "x86 instruction service listening on port " << port << std::endl;

  app.run();

  return 0;
}
