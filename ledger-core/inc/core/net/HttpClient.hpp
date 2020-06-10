/*
 *
 * HttpClient
 * ledger-core
 *
 * Created by Pierre Pollastri on 30/11/2016.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Ledger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <unordered_map>

#include <core/api/ExecutionContext.hpp>
#include <core/api/HttpClient.hpp>
#include <core/api/HttpMethod.hpp>
#include <core/api/HttpRequest.hpp>
#include <core/api/HttpReadBodyResult.hpp>
#include <core/api/HttpUrlConnection.hpp>
#include <core/async/Future.hpp>
#include <core/async/Promise.hpp>
#include <core/debug/Logger.hpp>
#include <core/net/HttpUrlConnectionInputStream.hpp>
#include <core/utils/Either.hpp>
#include <core/utils/Optional.hpp>
#include <core/utils/Option.hpp>

namespace ledger {
    namespace core {

        template <typename Success, typename Failure, class Handler>
        class HttpJsonHandler;

        class HttpRequest : public std::enable_shared_from_this<HttpRequest> {
        public:
            using JsonResult = std::tuple<std::shared_ptr<api::HttpUrlConnection>, std::shared_ptr<rapidjson::Document>>;

            HttpRequest(api::HttpMethod method,
                        const std::string& url,
                        const std::unordered_map<std::string, std::string>& headers,
                        const std::experimental::optional<std::vector<uint8_t >> body,
                        const std::shared_ptr<api::HttpClient> &client,
                        const std::shared_ptr<api::ExecutionContext> &context,
                        const Option<std::shared_ptr<spdlog::logger>>& logger);
            Future<std::shared_ptr<api::HttpUrlConnection>> operator()() const;

            template <typename Success, typename Failure, typename Handler>
            Future<Either<Failure, std::shared_ptr<Success>>> json(Handler handler) const {
                return operator()().recover(_context, [] (const Exception& exception) {
                    if (HttpRequest::isHttpError(exception.getErrorCode()) &&
                      exception.getUserData().nonEmpty()) {
                        return std::static_pointer_cast<api::HttpUrlConnection>(exception.getUserData().getValue());
                    }
                    throw exception;
                }).template map<Either<Failure, std::shared_ptr<Success>>>(_context, [handler] (const std::shared_ptr<api::HttpUrlConnection>& c) -> Either<Failure, std::shared_ptr<Success>> {
                    Handler h = handler;
                    std::shared_ptr<api::HttpUrlConnection> connection = c;
                    h.attach(connection);
                    HttpUrlConnectionInputStream is(connection);
                    rapidjson::Reader reader;
                    reader.Parse<rapidjson::ParseFlag::kParseNumbersAsStringsFlag>(is, h);
                    return (Either<Failure, std::shared_ptr<Success>>) h.build();
                });
            }

            Future<JsonResult> json(bool parseNumbersAsString = false) const;
            std::shared_ptr<api::HttpRequest> toApiRequest() const;


        private:
            api::HttpMethod _method;
            std::string _url;
            std::unordered_map<std::string, std::string> _headers;
            std::experimental::optional<std::vector<uint8_t >> _body;
            std::shared_ptr<api::HttpClient> _client;
            std::shared_ptr<api::ExecutionContext> _context;
            Option<std::shared_ptr<spdlog::logger>> _logger;

            static api::ErrorCode getErrorCode(int32_t statusCode) {
                return statusCode >= 200 && statusCode < 300 ? api::ErrorCode::FUTURE_WAS_SUCCESSFULL :
                statusCode >= 500 ? api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST :
                statusCode >= 400 ? api::ErrorCode::HTTP_ERROR :
                api::ErrorCode::TOO_MANY_REDIRECT;
            }

            static bool isHttpError(api::ErrorCode errorCode) {
                return errorCode == api::ErrorCode::HTTP_ERROR ||
                errorCode == api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST ||
                errorCode == api::ErrorCode::TOO_MANY_REDIRECT;
            }

            class ApiRequest : public api::HttpRequest {
            public:
                ApiRequest(const std::shared_ptr<const ledger::core::HttpRequest>& self);
                virtual api::HttpMethod getMethod() override;

                virtual std::unordered_map<std::string, std::string> getHeaders() override;

                virtual std::vector<uint8_t> getBody() override;

                virtual std::string getUrl() override;

                virtual ~ApiRequest();

                virtual void complete(const std::shared_ptr<api::HttpUrlConnection> &response,
                              const optional<api::Error> &error) override;

                Future<std::shared_ptr<api::HttpUrlConnection>> getFuture() const;

            private:
                std::shared_ptr<const ledger::core::HttpRequest> _self;
                Promise<std::shared_ptr<api::HttpUrlConnection>> _promise;
            };
        };

        class HttpClient {
        public:
            HttpClient(const std::string& baseUrl,
                       const std::shared_ptr<api::HttpClient> &client,
                       const std::shared_ptr<api::ExecutionContext> &context
            );
            HttpRequest GET(const std::string& path,
                            const std::unordered_map<std::string, std::string>& headers = {},
                            const std::string &baseUrl = "");
            HttpRequest PUT(const std::string& path, const std::vector<uint8_t> &body, const std::unordered_map<std::string, std::string>& headers = {});
            HttpRequest DEL(const std::string& path, const std::unordered_map<std::string, std::string>& headers = {});
            HttpRequest POST(
                const std::string& path,
                const std::vector<uint8_t> &body,
                const std::unordered_map<std::string, std::string>& headers = {},
                const std::string &baseUrl = "");
            HttpClient& addHeader(const std::string& key, const std::string& value);
            HttpClient& removeHeader(const std::string& key);
            void setLogger(const std::shared_ptr<spdlog::logger>& logger);

        private:
            HttpRequest createRequest(api::HttpMethod method,
                                      const std::string& path,
                                      const std::experimental::optional<std::vector<uint8_t >> body,
                                      const std::unordered_map<std::string, std::string>& headers,
                                      const std::string &baseUrl = "");

        private:
            std::string _baseUrl;
            std::shared_ptr<api::HttpClient> _client;
            std::shared_ptr<api::ExecutionContext> _context;
            std::unordered_map<std::string, std::string> _headers;
            Option<std::shared_ptr<spdlog::logger>> _logger;
        };
    }
}
