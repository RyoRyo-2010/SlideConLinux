#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

/*
 * ファイルが存在するか調べる．
 * path はファイルパス
 * 存在すれば0以外，存在しなければ0を返す
 */
int exist_file(const char *path) {
	FILE *fp = fopen(path,"r");
	if(fp == NULL) {
		return 0;
	}
	fclose(fp);
	return 1;
}

// 実行時引数，1つだけシリアルポートのパスを受け取る
int main(int argc, const char **argv) {
	struct termios tio;
	int serial_fd;
	const int baudRate = B9600;
	const int inputBufferSize = 64;

	if(argc != 2) {
		fprintf(stderr,"つかいかた: slidecon <シリアルポート>\n");
		return 1;
	}
	const char *SerialPath = argv[1];

	// シリアルポート存在するか
	if(!exist_file(SerialPath)) {
		fprintf(stderr,"シリアルポートのデバイスファイルが存在しません\n");
		return 2;
	}

	// シリアルポートを開く
	serial_fd = open(SerialPath,O_RDWR);
	if(serial_fd < 0) {
		fprintf(stderr,"シリアルポートのファイルをオープンにできませんでした\n");
		return 3;
	}
	// ここから，シリアルポートの設定(https://mcommit.hatenadiary.com/entry/2017/07/09/210840 )
	tio.c_cflag += CREAD;
	tio.c_cflag += CLOCAL;
	tio.c_cflag += CS8;
	tio.c_cflag += 0;
	tio.c_cflag += 0;

	cfsetispeed(&tio,baudRate);
	cfsetospeed(&tio,baudRate);

	cfmakeraw(&tio);

	tcgetattr(serial_fd,&tio);
	tcsetattr(serial_fd,TCSANOW,&tio);
	
	ioctl(serial_fd,TCSETS,&tio);
	// ここまで，シリアルポートの設定
	
	char input_buf[inputBufferSize];
	int  input_index = 0;
	int len;
	while(1) {
		// まずシリアルポートから読み込む
		len = read(serial_fd,input_buf+input_index,inputBufferSize-input_index);
		if(len <= 0) {
			continue;
		}
		printf("DEBUG: 受信しました．len=%d\n",len);
		input_index += len;
		input_buf[input_index] = '\0';
		printf("DEBUG: input_buf=%s, input_index=%d\n",input_buf,input_index);
		// バッファ溢れてないか？
		if(inputBufferSize - 10 <= input_index) {
			fprintf(stderr,"シリアルポート受信時に終端文字がなかったためバッファが溢れました．ポートを確認してください\n");
			input_index = 0;
			input_buf[input_index] = '\0';
		}

		// コマンドを読もう
		if(strncmp(input_buf,"left\n",inputBufferSize) == 0) {
			printf("DEBUG: LEFTがきたよ\n");
			input_index = 0;
			input_buf[input_index] = '\0';
		} else if(strncmp(input_buf,"right\n",inputBufferSize) == 0) {
			printf("DEBUG: RIGHTがきたよ\n");
			input_index = 0;
			input_buf[input_index] = '\0';
		}
	}
	close(serial_fd);
	return 0;
}
