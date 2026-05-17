/*
Slide Con Linux - Remote presentation controller

Copyright (C) 2026 RyoRyo-2010(https://github.com/RyoRyo-2010 )

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <linux/input.h>
#include "linux/uinput.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

const int inputBufferSize = 64;
const int BaudRate = B9600;
const char *UEventPath = "/dev/uinput";

void write_key(int code, int userInput_fd) {
	struct input_event key_event = {0};
	// 押して離す
	for(int i = 1;i >= 0;i--) {
		gettimeofday(&key_event.time,NULL);
		key_event.type = EV_KEY;
		key_event.code = code;
		key_event.value = i;
		if(write(userInput_fd,&key_event,sizeof(key_event)) 
			!= sizeof(key_event)) {
			fprintf(stderr,"User Inputの書き込みでエラーがおきました\n");
		}

		gettimeofday(&key_event.time,NULL);
		key_event.type = EV_SYN;
		key_event.code = SYN_REPORT;
		key_event.value = 0;
		if(write(userInput_fd,&key_event,sizeof(key_event))
			!= sizeof(key_event)) {
			fprintf(stderr,"User Inputの書き込みでエラーがおきました\n");
		}
	}
}

/*
 * コマンドを認識する．
 * leftかright
 */
void handle_command(const char *line, int userInput_fd) {
	if(strcmp(line,"left") == 0) {
		printf("LEFT命令を受信\n");
		write_key(KEY_LEFT,userInput_fd);
	}else if(strcmp(line,"right") == 0) {
		printf("RIGHT命令を受信\n");
		write_key(KEY_RIGHT,userInput_fd);
	}else {
		fprintf(stderr,"不明な命令を受信\n");
	}
}

/*
 * シリアルポートを準備する．
 * シリアルポートのパスを渡す．
 * 成功すればファイル記述子を返す．
 * 失敗すれば負の値を返す．
 */
int begin_serial( const char *SerialPath, struct termios *tio) {
	// シリアルポートを開く
	int serial_fd = open(SerialPath,O_RDWR);
	if(serial_fd < 0) {
		fprintf(stderr,"シリアルポートのファイルをオープンにできませんでした\n");
		return -3;
	}
	// ここから，シリアルポートの設定(参考: https://mcommit.hatenadiary.com/entry/2017/07/09/210840 )
	tcgetattr(serial_fd,tio);
	cfmakeraw(tio);
	tio->c_cflag |= (CREAD | CLOCAL | CS8);

	cfsetispeed(tio,BaudRate);
	cfsetospeed(tio,BaudRate);

	tcsetattr(serial_fd,TCSANOW,tio);
	
	ioctl(serial_fd,TCSETS,tio);
	// ここまで，シリアルポートの設定
	return serial_fd;
}

/*
 * User Inputの準備．
 * 成功すればファイル記述子を返す
 * 失敗すれば負の値を返す．
*/
int begin_userInput() {
	int fd = open(UEventPath,O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr,"%sが開けませんでした．権限は正しいですか？\n",UEventPath);
		return -1;
	}
	ioctl(fd,UI_SET_EVBIT,EV_KEY);
	ioctl(fd,UI_SET_KEYBIT,KEY_LEFT);
	ioctl(fd,UI_SET_KEYBIT,KEY_RIGHT);

	struct uinput_setup usetup = {0};
	snprintf(usetup.name,UINPUT_MAX_NAME_SIZE,"mykbd");
	usetup.id.bustype = BUS_USB;

	ioctl(fd,UI_DEV_SETUP,&usetup);
	ioctl(fd,UI_DEV_CREATE);
	return fd;
}

// 実行時引数，1つだけシリアルポートのパスを受け取る
int main(int argc, const char **argv) {
	struct termios tio;
	int serial_fd;
	int userInput_fd;
	printf("Slide Con Linux Ver1.0\n");
	printf("矢印キーを遠隔操作するシステムです\n");
	printf("---\n");

	if(argc != 2) {
		fprintf(stderr,"つかいかた: slidecon <シリアルポート>\n");
		return 1;
	}

	const char *SerialPath = argv[1];

	// シリアルポートの準備
	serial_fd = begin_serial(SerialPath,&tio);
	// エラーかな？
	if(serial_fd < 0) {
		return serial_fd;
	}

	// User Inputの準備
	userInput_fd = begin_userInput();
	if(userInput_fd < 0) {
		close(serial_fd);
		return userInput_fd;
	}

	char input_buf[inputBufferSize];
	int  input_index = 0;
	char c;
	while(read(serial_fd,&c,1) > 0) {
		if(c == '\n') {
			input_buf[input_index] = '\0';
			handle_command(input_buf,userInput_fd);
			input_index = 0;
		}else {
			if(input_index < inputBufferSize-1) {
				input_buf[input_index++] = c;
			}else {
				fprintf(stderr,"受信バッファが溢れました．バッファを破棄します\n");
				input_index = 0;
			}
		}
	}
	fprintf(stderr,"読めなくなったので終わります\n");
	close(serial_fd);
	
	ioctl(userInput_fd,UI_DEV_DESTROY);
	close(userInput_fd);
	return 0;
}
