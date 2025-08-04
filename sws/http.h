#ifndef HTTP_H
#define HTTP_H

int recv_http_req(FILE *in);
int send_http_resp(FILE *out);

#endif
