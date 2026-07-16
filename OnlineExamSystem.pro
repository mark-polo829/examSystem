TEMPLATE = subdirs

SUBDIRS += \
    server \
    client

server.file = server/server.pro
client.file = client/client.pro

# 确保服务端先编译
client.depends = server
