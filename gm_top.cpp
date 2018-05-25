//1552212 端启航
#include <iostream>
#include "MMX/RsaSignature.h"
#include "TCPServer.h"
#include "Member.h"
#include "GM.h"

using namespace std;


TCPServer *server;
group_sig::GM *gm;

string hardware_id;//id，由命令行输入

void send_r(string id, u_int8_t type, string msg = "") {
    auto Log = get("console");
    Log->info("GM sending response of type {0:x}...", type);
    header_t head;
    head.proto_ori = PROTO_S2C;
    head.proto_type = type;
    if (msg == "") {
        head.len = 0;
        server->SendPacket(id, (char *) &head, HEADLEN);
    } else {
        head.len = msg.size() + 1;
        char *buffer = new char[HEADLEN + msg.size() + 1];
        memcpy(buffer, &head, HEADLEN);
        memcpy(buffer + HEADLEN, msg.c_str(), msg.size() + 1);
        server->SendPacket(id, buffer, HEADLEN + msg.size() + 1);
    }
}


void onRecv_gm(ClientData *data) {
    header_t *header;
    header = (header_t *) (data->recv_playload);

    string msg, y, z, m, sig, vv;
    ZZ v;
    switch (header->proto_type) {
        case PROTO_PUB_PARA: {
            auto Log = get("console");
            Log->info("GM recv public para request");
            string id = get_str(data->recv_playload);
            data->id = id;
            group_sig::public_para p = gm->getPublicPara();
            header_t head;
            head.proto_ori = PROTO_S2C;
            head.proto_type = PROTO_PUB_PARA;
            string str;

            str += Cryptography::numberToString(p.a, false) + " ";
            str += Cryptography::numberToString(p.b, false) + " ";
            str += Cryptography::numberToString(p.epsilon, false) + " ";
            str += Cryptography::numberToString(p.G, false) + " ";
            str += Cryptography::numberToString(p.g, false) + " ";
            str += Cryptography::numberToString(p.n, false) + " ";
            str += to_string(p.lambda);
            head.len = str.size();

            char *buffer = new char[HEADLEN + head.len];
            memcpy(buffer, &head, HEADLEN);
            memcpy(buffer + HEADLEN, str.c_str(), head.len);
            server->SendPacket(data->id, buffer, HEADLEN + head.len);
            break;
        }
        case PROTO_JOIN_GROUP: {
            auto Log = get("console");
            Log->info("GM recv join group request");
            msg = get_str(data->recv_playload);
            v = gm->verify(data->id, msg);
            vv = Cryptography::numberToString(v, false);
            send_r(data->id, PROTO_JOIN_GROUP, vv);
            gm->keyExchangeRequest(data->id);
            break;
        }
        case PROTO_KEY_EX: {
            auto Log = get("console");
            Log->info("GM recv key exchg msg");
            msg = get_str(data->recv_playload);
            gm->onKeyExchangeResponseRecv(msg);
            msg = gm->getBroadcastMsg();
            // TODO
//            server->broadcast(msg, msg.size());
            break;
        }
        default:
            break;
    }
}

int main_gm(string ip, u_int16_t port, ZZ psk) {
    auto Log = get("console");
    Log->info("starting GM");
    gm = new group_sig::GM(1234, psk);
    server = new TCPServer(inet_addr(ip.c_str()), port);
    server->setOnRecvCallBack(onRecv_gm);
    server->StartServer();
    return 0;
}
