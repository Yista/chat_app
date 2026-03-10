// auth_handler.hpp
#ifndef AUTH_HANDLER_HPP
#define AUTH_HANDLER_HPP

#include <string>
#include <memory>
#include "database/user_dao.hpp"

class AuthHandler {
public:
    AuthHandler(std::shared_ptr<UserDAO> userDao);

    std::string handleLogin(const std::string& body);
    std::string handleRegister(const std::string& body);

private:
    std::shared_ptr<UserDAO> userDao_;
};

#endif