#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <sys/types.h> /* uint64_t */

void interrupt_stop_play();
void continue_play();

int get_play_status();

void unset_movi_finish_download();
int  is_movi_download_finished();

int tcp_serv_get_data_sock();


// Application interface functions

int init_tcp_serv();
int init_cmd_server();
void tcp_serv_finish();
void cmd_server_finish();
void player_add_write_bytes(int wrotebytes);
uint64_t player_get_read_bytes();
void set_movi_finish_download();
int is_tcp_server_running();
void player_set_movi_rate(double rate);
int is_player_ready();
void start_play();
void pause_play();
void stop_play();
void start_forced_play();

void player_set_file(const char * name);
#endif
