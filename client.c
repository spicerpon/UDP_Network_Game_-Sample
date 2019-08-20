/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_test.c,v 1.6 2004/10/19 22:01:27 dlowder Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ether.h> 
#include <net/ethernet.h> 
#include <net/if.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "define.h"

#define MAX_BUF 256
#define WIDTH 1024
#define HEIGHT 600
#define MAX_OBJ 10
#define GAME_OVER_SCORE 2000

pid_t pid;

#include "tslib.h"
#include "fbutils.h"

static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

/* [inactive] border fill text [active] border fill text */
static int button_palette [6] =
{
	7, 8, 2,
	3, 4, 5
};


struct ts_sample samp;
struct enemy_pos{
	int x, y;
};
int x, y;
int prev_status_player = 0;
int prev_status_enemy = 0;
struct enemy_pos enemy_pos;
struct sockaddr_in addr_server;
struct sockaddr_in addr_client;
socklen_t addr_server_len;
int sfd;
char buf[MAX_BUF];
char* SERVER_IP_PC = "192.168.10.2"; //Now, PC is Server
//Udp Variable

struct Object{
	int x, y, length, color, speed, dir;
};

struct Object player_obj[MAX_OBJ];
struct Object send_obj = {0,0,0,0,0,0};
struct Object enemy_obj[MAX_OBJ];

int player_code = 0;
int player_score = 0;
int enemy_score = 0;

int game_status = 0;

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static int get_rand(int max){
	srand(time( NULL));
	int num = rand() % max+10; 
	//printf("Random Num: %d\n", num);
	return num;
}

static void object_spawn (int sPosX, int sPosY, int length, int color, int speed, int dir, int status) {
   static int p_i = 0;
   static int e_i = 0;
   
   if(status)
   {
	   sPosY = get_rand(HEIGHT - 40) + 40;
	   length = get_rand(70) + 30;
	   color = get_rand(100) % 6;
	   speed = get_rand(20);
	   
	   if(player_code == 3){
		dir = 1;
		sPosX = get_rand(WIDTH / 2) + WIDTH / 2;
	   }
	   if(player_code == 4){
		dir = 0;
		sPosX = get_rand(WIDTH / 2);
	   }
	   
	   player_obj[p_i].x = sPosX;
	   player_obj[p_i].y = sPosY;
	   player_obj[p_i].length = length;
           player_obj[p_i].color = color;
	   player_obj[p_i].speed = speed;
	   player_obj[p_i].dir = dir;

	   rect (player_obj[p_i].x, player_obj[p_i].y, player_obj[p_i].x + player_obj[p_i].length, 
	   player_obj[p_i].y + player_obj[p_i].length, button_palette[player_obj[p_i].color] | XORMODE);   
	   fillrect (player_obj[p_i].x, player_obj[p_i].y, player_obj[p_i].x + player_obj[p_i].length, 
	   player_obj[p_i].y + player_obj[p_i].length, button_palette[player_obj[p_i].color] | XORMODE);	
	   
	   p_i++;
	   p_i %= MAX_OBJ;
	   printf("Draw New Player Object [x:%d  y:%d]\n", sPosX, sPosY);
   }else{
	enemy_obj[e_i].x = sPosX;
	enemy_obj[e_i].y = sPosY;
	enemy_obj[e_i].length = length;
        enemy_obj[e_i].color = color;
	enemy_obj[e_i].speed = speed;
	enemy_obj[e_i].dir = dir;

	rect (enemy_obj[e_i].x, enemy_obj[e_i].y, enemy_obj[e_i].x + enemy_obj[e_i].length, 
	enemy_obj[e_i].y + enemy_obj[e_i].length, button_palette[enemy_obj[e_i].color] | XORMODE);   
	fillrect (enemy_obj[e_i].x, enemy_obj[e_i].y, enemy_obj[e_i].x + enemy_obj[e_i].length, 
	enemy_obj[e_i].y + enemy_obj[e_i].length, button_palette[enemy_obj[e_i].color] | XORMODE);	
	
	e_i++;
	e_i %= MAX_OBJ;
	printf("Draw New Enemy Object [x:%d  y:%d]\n", sPosX, sPosY);
   }
 
} /* Rect Obejct Draw (Random Position[1024 * 600]) */

void* Thread_Spawn(void *arg) //Object Spawn
{	
	while(1)
	{
		usleep(get_rand(1500000) + 500000); //0.5 ~ 1.5 sec random 
		
		if(player_code == 0 || game_status == 0){
			printf("Waiting for Player Connect\n"); 
			continue;
		}

		object_spawn (0, 0, 0, 0, 0, 0, 1); //Player's Object
	}
}

void flush(int i, int type)
{
	if(type == 1){
		player_obj[i].x = 0;
		player_obj[i].y = 0;
		player_obj[i].length = 0;
		player_obj[i].speed = 0;
		player_obj[i].color = 0;
		player_obj[i].dir = 0;
	}else{
		enemy_obj[i].x = 0;
		enemy_obj[i].y = 0;
		enemy_obj[i].length = 0;
		enemy_obj[i].speed = 0;
		enemy_obj[i].color = 0;
		enemy_obj[i].dir = 0;
	}
}

int get_num(int n){
	return (n < 0) ? n * -1 : n; 
}

static void refresh_screen ();

int check_hit(int obj_x, int obj_y, int length)
{
	if(get_num(obj_x - x) < length + 20 && get_num(obj_y - y) < length + 20){
		player_score += length;
		printf("[%d] Score : %d\n", pid, player_score);
		refresh_screen ();
		return 1;
	}else{
		return 0;
	}
}

int hit_effect(int i, int status)
{
	while(status){
		rect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
		player_obj[i].y + player_obj[i].length, 1 | XORMODE);   

		fillrect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
		player_obj[i].y + player_obj[i].length, 1 | XORMODE);		
		
		usleep(200000);		

		rect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
		player_obj[i].y + player_obj[i].length, 1 | XORMODE);   

		fillrect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
		player_obj[i].y + player_obj[i].length, 1 | XORMODE);	
		
		break;
	}

	while(!status){
		rect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
		enemy_obj[i].y + enemy_obj[i].length, 1 | XORMODE);
   
		fillrect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
		enemy_obj[i].y + enemy_obj[i].length, 1 | XORMODE);		
	
		usleep(200000);		

		rect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
		enemy_obj[i].y + enemy_obj[i].length, 1 | XORMODE);
   
		fillrect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
		enemy_obj[i].y + enemy_obj[i].length, 1 | XORMODE);	
		break;
	}

}

void* Thread_Draw(void *arg) //Object Draw
{
	while(1)
	{
		usleep(40000);
		if(!game_status) continue;

		int i;
		for(i=0; i<MAX_OBJ; i++){
			if(player_obj[i].speed != 0){
				rect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
					player_obj[i].y + player_obj[i].length, button_palette[player_obj[i].color] | XORMODE);   
				fillrect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
					player_obj[i].y + player_obj[i].length, button_palette[player_obj[i].color] | XORMODE);

				if(player_obj[i].dir == 0) player_obj[i].x += player_obj[i].speed;
				if(player_obj[i].dir == 1) player_obj[i].x -= player_obj[i].speed;

				if(player_obj[i].x > WIDTH){
					
					if(player_code == 4){
						send_obj = player_obj[i];
					}
	
					flush(i, 1);
					continue;
				}else if(player_obj[i].x < 0){
					
					if(player_code == 3){
						send_obj = player_obj[i];	
					}					
		
					flush(i, 1);
					continue;
				}

				if(check_hit(player_obj[i].x, player_obj[i].y, player_obj[i].length) && game_status){
					hit_effect(i, 1);
					flush(i, 1);
					continue;
				};
					
				rect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
					player_obj[i].y + player_obj[i].length, button_palette[player_obj[i].color] | XORMODE);   
				fillrect (player_obj[i].x, player_obj[i].y, player_obj[i].x + player_obj[i].length, 
					player_obj[i].y + player_obj[i].length, button_palette[player_obj[i].color] | XORMODE); 

			}
			
			if(enemy_obj[i].speed != 0){
				rect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
				enemy_obj[i].y + enemy_obj[i].length, button_palette[enemy_obj[i].color] | XORMODE);   
				fillrect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
				enemy_obj[i].y + enemy_obj[i].length, button_palette[enemy_obj[i].color] | XORMODE);	

				if(enemy_obj[i].dir == 0) enemy_obj[i].x += enemy_obj[i].speed;
				if(enemy_obj[i].dir == 1) enemy_obj[i].x -= enemy_obj[i].speed;

				if(enemy_obj[i].x > WIDTH || enemy_obj[i].x < 0){
					flush(i, 0);
					continue;
				}

				if(check_hit(enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].length) && game_status){
					hit_effect(i, 0);
					flush(i, 0);
					continue;
				};

				rect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
					enemy_obj[i].y + enemy_obj[i].length, button_palette[enemy_obj[i].color] | XORMODE);   
				fillrect (enemy_obj[i].x, enemy_obj[i].y, enemy_obj[i].x + enemy_obj[i].length, 
					enemy_obj[i].y + enemy_obj[i].length, button_palette[enemy_obj[i].color] | XORMODE);
					
			}
		}
	}
}

void Thread_UDP(void *arg)
{
	static int prev_status;

	while(1)
	{
		int udp_ret;
		int len;
		sfd = socket(AF_INET, SOCK_DGRAM, 0);
		udp_ret = bind(sfd, (struct sockaddr *)&addr_client, sizeof(addr_client));
	
		if(udp_ret == -1) { 
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		
		char msg[100];
		/* set send_obj and player info like packet*/
		sprintf(msg, "%d%s%d%s%d%s%d%s%d%s%d%s%d", samp.x, " ", samp.y, " ", player_score, " ", send_obj.y, " ", send_obj.length, " ", send_obj.color, " ", send_obj.speed);
		
        	//printf("[%d] Position Send Attemped : %s\n", pid, msg);
		sendto(sfd, msg, strlen(msg), 0, (struct sockaddr *)&addr_server, sizeof(addr_server));

		//Wating for Server Respone
		int rcv_status = 1;		

		while(rcv_status){
			addr_server_len = sizeof(addr_server);
			len = recvfrom(sfd, buf, MAX_BUF-1, 0, (struct sockaddr *)&addr_server, &addr_server_len);
		
			if(len > 0) {
				buf[len] = 0;
				//printf("[%d] received: %s\n", pid, buf);
				//printf("[%d] player code: %d\n", pid, player_code);
				//printf("[%d] enemy_score %d\n", pid, enemy_score);
				int pos[40];
				int idx = 0;
				char *ptr = strtok(buf, " "); 
 
				while (ptr != NULL){
					//printf("%s ", ptr);
					pos[idx++] = atoi(ptr);					         
					ptr = strtok(NULL, " ");
				}

				enemy_score = pos[2];
				player_code = pos[7];

				if((pos[3] && pos[4] && pos[5] && pos[6]) && prev_status != pos[3] * 1 + pos[4] * 2 + pos[5] * 3 + pos[6] * 4){			
					/* pos[3] ~ [6] -> enemy send_object infomation */
					int dir = 0;
					int sPosX;
					if(player_code == 4) dir = 1; else dir = 0; 
					if(player_code == 4) sPosX = WIDTH - 5; else sPosX = 5;
					
					object_spawn (sPosX, pos[3], pos[4], pos[5], pos[6], dir, 0); //Enemy's Object
					
					prev_status = pos[3] * 1 + pos[4] * 2 + pos[5] * 3 + pos[6] * 4;
					//printf("[%d] prev_status: %d\n", pid, prev_status);
				}

				if(enemy_score < 0) enemy_score = 0;

				if (prev_status_enemy == 0){
					put_cross2(enemy_pos.x, enemy_pos.y, 4 | XORMODE); 
				}				
			
				if(enemy_pos.x == pos[0] && enemy_pos.y == pos[1]) prev_status_enemy = 1;
				else{
					prev_status_enemy = 0;
				}	
				
				if (prev_status_enemy == 0){
					put_cross2(enemy_pos.x, enemy_pos.y, 4 | XORMODE); 
				}
				
				enemy_pos.x = pos[0];
				enemy_pos.y = pos[1];				

				rcv_status = 0;
			}
		}
		//printf("[%d] Position Send Completed, closed\n", pid);	
		close(sfd);		
	}
}
	
static void refresh_screen ()
{
	int i=0;
	
	char player1_score[BUFSIZ];
	char player2_score[BUFSIZ];
	
	fillrect (0, 0, xres - 1, 40, 0);
	put_string_center (xres/2, yres/16 - 20,   "YDH test program ver 1.0", 1);
	put_string_center (xres/2, yres/16,"Touch screen to move crosshair", 2);

	if(player_code == 4){
		sprintf(player1_score, "%s%d", "", player_score);
		sprintf(player2_score, "%s%d", "", enemy_score);  
	}else if(player_code == 3){
 		sprintf(player1_score, "%s%d", "", enemy_score);
		sprintf(player2_score, "%s%d", "", player_score);
	}
	

	put_string_center (xres/2 - 450, yres/16 - 20, "Player 1", 1);
	put_string_center (xres/2 - 450, yres/16, player1_score, 2);
	put_string_center (xres/2 + 450, yres/16 - 20, "Player 2", 1);
	put_string_center (xres/2 + 450, yres/16, player2_score, 2);

	/* for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]); //Button Draw */
}

int main()
{        
	game_status = 1; /* GAME START */
	
	send_obj.y = 0;
	send_obj.length = 0;
	send_obj.speed = 0;
	send_obj.color = 0;

	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = inet_addr(SERVER_IP_PC);
	addr_server.sin_port = htons(SERVER_PORT);
	
	printf("[%d] SERVER_IP: %s\n", pid, SERVER_IP_PC);
	printf("[%d] SERVER_PORT: %d\n", pid, SERVER_PORT); 
	//Udp Setting Check

	if(sfd == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}   

	memset(&addr_client, 0, sizeof(addr_client));
	addr_client.sin_family = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port = 0; /* A random number is assigned */
	//Udp Setting	


	struct tsdev *ts;
	unsigned int i;
	unsigned int mode = 0;

	char *tsdevice=NULL;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	if ((tsdevice = getenv("TSLIB_TSDEVICE")) == NULL) {
#ifdef USE_INPUT_API
		tsdevice = strdup ("/dev/input/event0");
#else
		tsdevice = strdup ("/dev/touchscreen/ucb1x00");
#endif /* USE_INPUT_API */
        }

	ts = ts_open (tsdevice, 1);

	if (!ts) {
		perror (tsdevice);
		exit(1);
	}

	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	x = xres/2;
	y = yres/2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	/*memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = xres / 4;
	buttons [0].h = buttons [1].h = 20;
	buttons [0].x = xres / 4 - buttons [0].w / 2;
	buttons [1].x = (3 * xres) / 4 - buttons [0].w / 2;
	buttons [0].y = buttons [1].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";*/

	refresh_screen ();	
	
	pthread_t thread_n, thread_s, thread_d;
	pthread_create(&thread_n, NULL, Thread_UDP, NULL); /* UDP Respone Thread */
	pthread_create(&thread_s, NULL, Thread_Spawn, NULL); /* Obejct Spawn Thread */
	pthread_create(&thread_d, NULL, Thread_Draw, NULL); /* Obejct Draw Thread */

	for (;;) {
		if(player_score >= GAME_OVER_SCORE || enemy_score >= GAME_OVER_SCORE){
			game_status = 0;
			printf("GAME OVER!\n");

			refresh_screen (); 

			fillrect (0, 40, xres - 1, yres - 1, 0);
			put_string_center (xres/2, yres/2 + 15, "GAME OVER", 1);
			
			if(player_code == 4){
				if(player_score > enemy_score)
					put_string_center (xres/2, yres/2 - 15, "Player 1 Win!", 2);
				else put_string_center (xres/2, yres/2 - 15, "Player 2 Win!", 2);
			}else if(player_code == 3){
				if(player_score > enemy_score)
					put_string_center (xres/2, yres/2 - 15, "Player 2 Win!", 2);
				else put_string_center (xres/2, yres/2 - 15, "Player 1 Win!", 2);
			}
	
			break;
		}

		int ret;
		
		/* Show the cross */
		if (prev_status_player == 0){
			put_cross2(x, y, 2 | XORMODE);
		}		

		ret = ts_read(ts, &samp, 1);
		
		if(x == samp.x && y == samp.y) prev_status_player = 1;
		else {
			prev_status_player = 0;		
		}

		/* Hide it */
		if (prev_status_player == 0){
			put_cross2(x, y, 2 | XORMODE);
		}


		if (ret < 0) {
			perror("ts_read");
			printf("ts_read close.\n");
			close_framebuffer();
			exit(1);
		}
	
		if (ret != 1){	
			continue;
		}

		/*for (i = 0; i < NR_BUTTONS; i++)
			if (button_handle (&buttons [i], &samp))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen ();
					break;
				case 1:
					mode = 1;
					refresh_screen ();
					break;
				}*/

		printf("%ld.%06ld: x:%6d y:%6d %6d ex:%6d ey:%6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure, enemy_pos.x, enemy_pos.y);
	
             
		if (samp.pressure >= 0) {
			//if (mode == 0x80000001) line (x, y, samp.x, samp.y, 2);
			x = samp.x;
			y = samp.y;
			//mode |= 0x80000000;
		} else {
			//mode &= ~0x80000000;
		}
	}
	close_framebuffer();
}
