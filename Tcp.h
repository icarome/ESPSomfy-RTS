#ifndef tcp_h
#define tcp_h
#include <AsyncTCP.h>
#include <vector>
#include <Arduino.h>
extern std::vector<AsyncClient*> clients;
static char message[1024];
static void wifi_debug(String tosend){
  if(clients.size()>0){
    char reply[1024];
    tosend.toCharArray(reply, tosend.length()+1);
    reply[tosend.length()+1] = '\r';
    char new_line[1];
    new_line[0] = '\n';
    for(uint32_t i = 0; i<clients.size(); i++){
      if (clients[i]->space() > 32 && clients[i]->canSend()) {
        clients[i]->add(reply, strlen(reply)+1);
        clients[i]->send();
        clients[i]->write(new_line, 1);
      }
    }
  }
}
#endif
