#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "hw2.h"

#define ERR_EXIT(s) perror(s), exit(errno);
#define MAX_NODES 100
// somethings I recommend leaving here, but you may delete as you please
static char root[MAX_FRIEND_INFO_LEN] = "Not_Tako";     // root of tree
static char friend_info[MAX_FRIEND_INFO_LEN];   // current process info
static char friend_name[MAX_FRIEND_NAME_LEN];   // current process name
static int friend_value;    // current process value
FILE* read_fp = NULL;

// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(friend_name, root) == 0);
}

typedef struct Node {
    char name[MAX_FRIEND_NAME_LEN];
    struct Node *first_child;
    struct Node *next_sibling;
} Node;
typedef struct Queue{
	Node *nodes[100];
	int front;
	int rear;
}Queue;

void init_queue(Queue *q){
	q -> front = 0;
	q -> rear = 0;
}
void enqueue(Queue *q, Node *node){
	q -> nodes[q->rear++] = node;
}
Node *dequeue(Queue *q)	{
	return q->nodes[q->front++];
}
int is_empty(Queue *q){
	return q->front == q->rear;
}

// a bunch of prints for you
void print_direct_meet(char *friend_name) {
    fprintf(stdout, "Not_Tako has met %s by himself\n", friend_name);
}

void print_indirect_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako has met %s through %s\n", child_friend_name, parent_friend_name);
}

void print_fail_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako does not know %s to meet %s\n", parent_friend_name, child_friend_name);
}

void print_fail_check(char *parent_friend_name){
    fprintf(stdout, "Not_Tako has checked, he doesn't know %s\n", parent_friend_name);
}

void print_success_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s has adopted %s\n", parent_friend_name, child_friend_name);
}

void print_fail_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s is a descendant of %s\n", parent_friend_name, child_friend_name);
}

void print_compare_gtr(char *friend_name){
    fprintf(stdout, "Not_Tako is still friends with %s\n", friend_name);
}

void print_compare_leq(char *friend_name){
    fprintf(stdout, "%s is dead to Not_Tako\n", friend_name);
}

void print_final_graduate(){
    fprintf(stdout, "Congratulations! You've finished Not_Tako's annoying tasks!\n");
}
Node* create_node(const char *name) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    strncpy(new_node->name, name, MAX_FRIEND_NAME_LEN - 1);
    new_node->name[MAX_FRIEND_NAME_LEN - 1] = '\0';
    new_node->first_child = NULL;
    new_node->next_sibling = NULL;
    return new_node;
}

Node* find_node(Node *root, const char *name) {
    if (root == NULL) return NULL;
    if (strcmp(root->name, name) == 0) return root;

    Node *child_result = find_node(root->first_child, name);
    if (child_result != NULL) return child_result;

    return find_node(root->next_sibling, name);
}

void add_child(Node *parent, Node *child) {
    if (parent->first_child == NULL) {
        parent->first_child = child;
    } else {
        Node *sibling = parent->first_child;
        while (sibling->next_sibling != NULL) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }
}
void print_tree(Node *node){
	if(node ==NULL)return;

	Queue q;
	init_queue(&q);
	enqueue(&q, node);
	
	while(!is_empty(&q)){
		int level_size = q.rear - q.front;
		
		for(int i=0;i<level_size;i++){
			Node *current = dequeue(&q);

			printf("%s",current -> name);
			if(i<level_size -1){
				printf(" ");
			}
			Node *child = current -> first_child;
			while(child != NULL){
				enqueue(&q, child);
				child = child->next_sibling;
			}
		}
		printf("\n");
	}
}

void meet(Node *root, char *parent_friend_name, char *child_friend_info) {
    Node *parent_node = find_node(root,parent_friend_name);
    
    if (parent_node == NULL) {
        // 如果 parent_friend_name 不存在於樹中，顯示錯誤信息
        print_fail_meet((char *)parent_friend_name, (char *)child_friend_info);
        return;
    }

    Node *child_node = create_node(child_friend_info);

    int pipe_parent_to_child[2], pipe_child_to_parent[2];
    char buffer[1024];

    // 創建兩個管道
    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        ERR_EXIT("pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        // 錯誤處
        ERR_EXIT("fork");
    } else if (pid == 0) {
        // 子進程代碼
        close(pipe_parent_to_child[1]); // 關閉父進程寫的端點
        close(pipe_child_to_parent[0]); // 關閉父進程讀的端點

        // 重定向子進程的標準輸入和輸出到管道
        dup2(pipe_parent_to_child[0], STDIN_FILENO);
        dup2(pipe_child_to_parent[1], STDOUT_FILENO);

        // 關閉重定向後的管道端點
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        // 執行新的程序
        execlp("./friend", "friend", child_friend_info, (char *)NULL);
        ERR_EXIT("execlp"); // execlp 失敗時退出
    } else {
        // 父進程代碼
        close(pipe_parent_to_child[0]); // 關閉子進程讀的端點
        close(pipe_child_to_parent[1]); // 關閉子進程寫的端點

	add_child(parent_node, child_node);

        // 檢查 parent_friend_name 是否為 "Not_Tako"
        if (strcmp(parent_friend_name, "Not_Tako") == 0) {
            // 直接見面
            print_direct_meet((char *)child_friend_info);
        } else {
            // 間接見面
            print_indirect_meet((char *)parent_friend_name, (char *)child_friend_info);
        }

        // 父進程可以寫入管道，發送命令給子進程（可以根據需求進行修改）
        write(pipe_parent_to_child[1], "Hello, child\n", 13);

        // 從子進程讀取回應
        ssize_t n = read(pipe_child_to_parent[1], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Parent received: %s\n", buffer);
        }

        // 關閉管道並等待子進程結束
        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);
        waitpid(pid, NULL, 0);
    }
}
void check(Node *root,char *parent_friend_name){
	Node *parent_node = find_node(root, parent_friend_name);
	
        if(parent_node == NULL){
		print_fail_check(parent_friend_name);
		return;
	}
	print_tree(parent_node);
}

int main(int argc, char *argv[]) {
    // Hi! Welcome to SP Homework 2, I hope you have fun
    //pid_t process_pid = getpid(); // you might need this when using fork()
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [friend_info]\n");
        return 0;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // prevent buffered I/O, equivalent to fflush() after each stdout, study this as you may need to do it for other friends against their parents
    
    // put argument one into friend_info
    strncpy(friend_info, argv[1], MAX_FRIEND_INFO_LEN);
    Node *root_node = NULL;
    if(strcmp(argv[1], root) == 0){
        // is Not_Tako
	root_node = create_node("Not_Tako");
        strncpy(friend_name, friend_info,MAX_FRIEND_NAME_LEN);      // put name into friend_nae
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';        // in case strcmp messes with you
        read_fp = stdin;        // takes commands from stdin
        friend_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
    }
    char command[100];
    while (fgets(command, sizeof(command), read_fp)) {
            command[strcspn(command, "\n")] = '\0'; // 去掉換行符號

            if (strncmp(command, "Meet", 4) == 0) {
                // 使用 strtok 分割字符串
                char *token = strtok(command, " ");
                token = strtok(NULL, " ");  // 取得 parent_friend_name
                char parent_friend_name[MAX_FRIEND_NAME_LEN];
                if (token != NULL) {
                    strncpy(parent_friend_name, token, MAX_FRIEND_NAME_LEN - 1);
                    parent_friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';
                } else {
                    fprintf(stderr, "Invalid Meet command format: %s\n", command);
                    continue;
                }

                token = strtok(NULL, " ");  // 取得 child_friend_info
                char child_friend_info[MAX_FRIEND_INFO_LEN];
                if (token != NULL) {
                    strncpy(child_friend_info, token, MAX_FRIEND_INFO_LEN - 1);
                    child_friend_info[MAX_FRIEND_INFO_LEN - 1] = '\0';
                } else {
                    fprintf(stderr, "Invalid Meet command format: %s\n", command);
                    continue;
                }

                //printf("Parsed parent_friend_name: %s, child_friend_info: %s\n", parent_friend_name, child_friend_info);
                meet(root_node, parent_friend_name, child_friend_info);
            }else if (strncmp(command, "Check", 5) == 0) {
 		   // 使用 sscanf 解析 Check 指令
    		char parent_friend_name[MAX_FRIEND_NAME_LEN];
    		if (sscanf(command, "Check %s", parent_friend_name) == 1) {
        		// 調用 check 函數
        		check(root_node, parent_friend_name);
    		} else {
        		fprintf(stderr, "Invalid Check command format: %s\n", command);
    			}
		}	
	}
        
        // 這裡可以加入其他指令的處理邏輯
    if(is_Not_Tako()){
        print_final_graduate();
    }
    return 0;
}
