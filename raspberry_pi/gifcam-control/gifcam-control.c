#include <wiringPi.h>

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <pthread.h>

#define SHUTDOWN_PIN 29       // pin 40, wiring pi 29
#define ALIVE_PIN 25       // pin 37, wiring pi 25
#define BUT_LED_PIN 26       // pin 32, wiring pi 26
#define BUTTON_PIN 24       // pin 35, wiring pi 24

void alive() {
	printf("gifcam-control: send alive signal\n");
	pinMode (ALIVE_PIN, OUTPUT);
	digitalWrite (ALIVE_PIN, HIGH);
	delay (50);
	digitalWrite (ALIVE_PIN,  LOW);
}

void button_led_on() {
	printf("gifcam-control: set button led on\n");
	pinMode (BUT_LED_PIN, OUTPUT);
	digitalWrite (BUT_LED_PIN, HIGH);
}

void button_led_off() {
	printf("gifcam-control: set button led off\n");
	pinMode (BUT_LED_PIN, OUTPUT);
	digitalWrite (BUT_LED_PIN, LOW);
}

void * processing_video(void *data) {
	char cmd[1024];
	char *filename = (char *)data;

	// concat with reverse itself
	system("ffmpeg -r 1 -s 1280x720 -pix_fmt yuv420p -i /tmp/frame%03d.raw -c:v rawvideo -pix_fmt yuv420p -filter_complex \"[0:v]reverse,fifo[r];[0:v][r] concat=n=2:v=1 [v]\" -map \"[v]\" -y /tmp/rev.avi");
	// interpolate and encode to mp4
	sprintf(cmd, "ffmpeg -r 8 -i /tmp/rev.avi -c:v h264_omx -filter \"minterpolate='mi_mode=blend:fps=25'\" -b:v 3000000 -y %s", filename);
	system(cmd);

	return NULL;
}

void * blink_button(void *data) {
	while (1) {
		button_led_off();
		sleep(1);
		button_led_on();
		sleep(1);
	}
	return NULL;
}

void monitor() {
	printf("gifcam-control: monitor\n");

	pinMode (SHUTDOWN_PIN, INPUT);
	pullUpDnControl (SHUTDOWN_PIN, PUD_UP);

	pinMode (BUTTON_PIN, INPUT);
	pullUpDnControl (BUTTON_PIN, PUD_DOWN);

	do {
		if (digitalRead(SHUTDOWN_PIN) == 0) {
			button_led_off();
			printf("gifcam-control: shutting down...\n");
			system("shutdown -h now");
		}

		if (digitalRead(BUTTON_PIN) == 1) {

			pthread_t process_vid_thread;
			pthread_t blink_but_thread;

			char cmd[1024];
			char filename[1024];
			struct tm *timenow;

			time_t now = time(NULL);
			timenow = gmtime(&now);
			strftime(filename, sizeof(filename), "/media/videos/vid_%Y%m%d%H%M%S.mp4", timenow);

			button_led_on();
			printf("gifcam-control: taking vid...\n");

			system("rm -f /tmp/frame*.raw");
			system("rm -f /tmp/rev.avi");

			// capture some pictures
			system("raspiyuv -bm -ISO 800 -rot 90 -w 1280 -h 720 -t 5000 -tl 0 -o /tmp/frame%03d.raw");


			int ret;
			ret = pthread_create (
			          &process_vid_thread, NULL,
			          processing_video, filename
			      );

			ret = pthread_create (
			          &blink_but_thread, NULL,
			          blink_button, NULL
			      );

			pthread_join (process_vid_thread, NULL);
			pthread_cancel(blink_but_thread);


			delay(50);
			button_led_off();
		}

		delay(1000);
	}
	while (1);
}

int main (int argc, char **argv)
{
	wiringPiSetup();

	if (argc > 1) {
		const char *cmd = argv[1];
		if (!strcmp(cmd, "monitor")) {
			monitor();
		} else if (!strcmp(cmd, "alive")) {
			alive();
		} else if (!strcmp(cmd, "button_led_on")) {
			button_led_on();
		} else if (!strcmp(cmd, "button_led_off")) {
			button_led_off();
		} else {
			printf("gifcam-control: unknown command\n");
		}

	} else {
		printf("Usage: gifcam-control [monitor|alive|button_led_on|button_led_off]\n");
	}
}
