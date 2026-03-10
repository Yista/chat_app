#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

struct Message {
    int id = 0;
    int senderId = 0;
    int receiverId = 0;
    std::string senderName;   // 新增：发送者用户名
    std::string content;
    int msgType = 0;          // 0文本, 1图片, 2文件
    int status = 0;           // 0正常, 1撤回, 2删除
    std::string createdAt;
};

#endif