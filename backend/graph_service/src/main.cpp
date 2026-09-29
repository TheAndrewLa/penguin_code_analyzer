#include <drogon/drogon.h>

#include "x86_graph.hpp"

#include <cstddef>
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

drogon::HttpResponsePtr MakeJsonError(const std::string &message, drogon::HttpStatusCode code) {
  Json::Value body;
  body["error"] = message;

  auto response = drogon::HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(code);

  return response;
}

llvm::Expected<std::vector<std::byte>> ExtractBinary(const drogon::HttpRequestPtr &req) {
  if (req->getContentType() == drogon::CT_MULTIPART_FORM_DATA) {
    drogon::MultiPartParser parser;

    if (parser.parse(req) != 0) {
      return llvm::createStringError("Can not parse multipart form data!");
    }

    const auto &files = parser.getFiles();

    if (files.empty()) {
      return llvm::createStringError("Request must contain a file with binary data!");
    }

    const auto content = files.front().fileContent();
    const auto *begin = reinterpret_cast<const std::byte *>(content.data());

    return std::vector<std::byte>(begin, begin + content.size());
  }

  const auto body = req->getBody();

  if (body.empty()) {
    return llvm::createStringError("Request body must not be empty!");
  }

  const auto *begin = reinterpret_cast<const std::byte *>(body.data());

  return std::vector<std::byte>(begin, begin + body.size());
}
} // namespace

int main(int argc, char *argv[]) {
  auto port = DefaultPort;

  for (int i = 1; i < argc; ++i) {
    const auto arg = std::string(argv[i]);

    if (arg.rfind("--port=", 0) == 0) {
      port = std::stoi(arg.substr(7));
    }
  }

  auto service = X86GraphBuilder();
  auto &app = drogon::app();

  app.addListener("0.0.0.0", port);
  app.setThreadNum(std::thread::hardware_concurrency());
  app.setClientMaxBodySize(MaxBodySize);

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
        auto binary = ExtractBinary(req);

        if (!binary) {
          callback(MakeJsonError(llvm::toString(binary.takeError()), drogon::k400BadRequest));
          return;
        }

        auto result = service.start(binary->data(), binary->size());

        if (!result) {
          callback(MakeJsonError(llvm::toString(result.takeError()), drogon::k400BadRequest));
          return;
        }

        auto body = Json::Value(Json::objectValue);
        body["session"] = result->session;

        auto functions = Json::Value(Json::arrayValue);
        for (const auto &name : result->functions) {
          functions.append(name);
        }
        body["functions"] = std::move(functions);

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
      },
      {drogon::Post});

  app.registerHandler(
      "/getGraph",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        const auto session = req->getParameter("session");
        const auto name = req->getParameter("name");

        if (session.empty()) {
          callback(MakeJsonError("Query parameter 'session' is not set!", drogon::k400BadRequest));
          return;
        }

        if (name.empty()) {
          callback(MakeJsonError("Query parameter 'name' is not set!", drogon::k400BadRequest));
          return;
        }

        auto graph = service.getFunctionGraph(session, name);

        if (!graph) {
          callback(MakeJsonError(llvm::toString(graph.takeError()), drogon::k400BadRequest));
        } else {
          callback(drogon::HttpResponse::newHttpJsonResponse(*graph));
        }
      },
      {drogon::Get});

  app.registerHandler(
      "/end",
      [&service](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        const auto session = req->getParameter("session");

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

  std::cout << "x86 graph service listening on port " << port << std::endl;

  app.run();

  return 0;
}
