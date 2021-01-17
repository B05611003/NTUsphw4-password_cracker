#include <openssl/md5.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

jmp_buf MAIN;
jmp_buf thread[10];

struct arg_struct {
	char prefix[256];
	char goal[10];
	char ***ans;
	int N;
	int M;
	int i;
};

void printf_md5(unsigned char *in) {
	int i;
	for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
		printf("%02x", in[i]);
	}
	printf("\n");
}

int hashcmp(unsigned char *hash, char *goal, int count) {
	char hashstring[7];
	sprintf(hashstring, "%02x%02x%02x", hash[0], hash[1], hash[2]);
	return !strncmp(goal, hashstring, count);
}

void findfirst(char *prefix, char *goal, char ***ans, int *cur_count, int goal_count, int index, int iter_level, int passwd_level, jmp_buf point) {
	if (iter_level == 0) {
		prefix[index] = (char)0;
		if (hashcmp(MD5(prefix, strlen(prefix), NULL), goal, passwd_level)) {
			strcpy(ans[*cur_count][passwd_level - 1], prefix);
			// printf("finish\n");
			*cur_count += 1;
			if (*cur_count == goal_count) {
				longjmp(point, 1);
			}
		}
		return;
	}
	for (int i = 32; i < 127; i++) {
		prefix[index] = (char)i;
		findfirst(prefix, goal, ans, cur_count, goal_count, index + 1, iter_level - 1, passwd_level, point);
	}
}

void *treasure(void *data) {
	struct arg_struct *args = data;
	int now_iter;
	int id;
	for (int passwd_level = 2; passwd_level <= args->N; passwd_level++) {
		now_iter = 1;
		id = args->i;
		if (setjmp(thread[args->i]) == 0) {
			while (1) {
				findfirst(args->prefix, args->goal, args->ans, &id, args->i + 1, strlen(args->prefix), now_iter, passwd_level, thread[args->i]);
				now_iter++;
			}
		}
	}
	pthread_exit(NULL);
}

// ./main prefix goal N M file
// prefix < 128
// goal < 5
// N < 5
// M < 10
int main(int argc, char *argv[]) {
	char prefix[256];
	char goal[10];
	int N;
	int M;

	strcpy(prefix, argv[1]);
	strcpy(goal, argv[2]);
	N = atoi(argv[3]);
	M = atoi(argv[4]);

	pthread_t tid[M];

	char ***ans = malloc(M * sizeof(char **));
	for (int i = 0; i < M; i++) {
		ans[i] = (char **)malloc(N * sizeof(char *));
		for (int j = 0; j < N; j++) {
			ans[i][j] = (char *)malloc(256 * sizeof(char));
		}
	}

	int count = 0;
	int now_iter = 1;
	int original_len = strlen(prefix);
	if (setjmp(MAIN) == 0) {
		while (1) {
			findfirst(prefix, goal, ans, &count, M, original_len, now_iter, 1, MAIN);
			now_iter++;
		}
	}
	struct arg_struct argu[M];
	for (int i = 0; i < M; i++) {
		strcpy(argu[i].prefix, ans[i][0]);
		strcpy(argu[i].goal, goal);
		argu[i].M = M;
		argu[i].N = N;
		argu[i].i = i;
		argu[i].ans = ans;

		pthread_create(&tid[i], NULL, treasure, (void *)&argu[i]);
	}
	for (int i = 0; i < M; i++) pthread_join(tid[i], NULL);
	FILE *pt;
	pt = fopen(argv[5],"w");

	for (int i = 0; i < M; i++) {
		for (int j = 0; j < N; j++) {
			fprintf(pt,"%s\n",ans[i][j]);
		}
		fprintf(pt,"===\n");
	}
	return 0;
}
