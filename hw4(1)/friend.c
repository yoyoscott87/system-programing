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
void get_friend_name(const char *friend_info, char *friend_name);
int get_friend_value(const char *friend_info);
// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(friend_name, root) == 0);
}

typedef struct Node {
    char friend_info[MAX_FRIEND_INFO_LEN];
    struct Node *first_child;
    struct Node *next_sibling;
    pid_t pid;
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];
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

Node* create_node(const char *friend_name, int friend_value) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (strcmp(friend_name, "Not_Tako") == 0) {
        // 如果是 Not_Tako，忽略 friend_value
        snprintf(new_node->friend_info, MAX_FRIEND_INFO_LEN, "%s", friend_name);
    } else {
        // 其他節點包含 friend_name 和 friend_value
        snprintf(new_node->friend_info, MAX_FRIEND_INFO_LEN, "%s_%02d", friend_name, friend_value);
    }
    new_node->pid = -1;
    new_node->first_child = NULL;
    new_node->next_sibling = NULL;
    return new_node;

}

Node* find_node(Node *root, const char *target_name) {
    if (root == NULL) return NULL;

    // 提取節點的 friend_name 部分
    char current_name[MAX_FRIEND_NAME_LEN];
    get_friend_name(root->friend_info, current_name);

    // 僅根據 friend_name 進行匹配
    if (strcmp(current_name, target_name) == 0) return root;

    // 遞迴查找子節點
    Node *child_result = find_node(root->first_child, target_name);
    if (child_result != NULL) return child_result;

    // 遞迴查找兄弟節點
    return find_node(root->next_sibling, target_name);
}

void get_friend_name(const char *friend_info, char *friend_name) {
    char temp[MAX_FRIEND_INFO_LEN];
    strncpy(temp, friend_info, MAX_FRIEND_INFO_LEN);
    char *underscore_pos = strchr(temp, '_');
    if (underscore_pos != NULL) {
        *underscore_pos = '\0';
        strncpy(friend_name, temp, MAX_FRIEND_NAME_LEN - 1);
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';
    }
}
int get_friend_value(const char *friend_info) {
    char *underscore_pos = strchr(friend_info, '_');
    if (underscore_pos != NULL) {
        return atoi(underscore_pos + 1);
    }
    return -1;  // 如果格式不正確，返回 -1
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

			printf("%s",current -> friend_info);
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
void meet(Node *root, char *parent_friend_name, const char *child_friend_info) {
    Node *parent_node = find_node(root, parent_friend_name);
    
    if (strcmp(parent_friend_name, "Not_Tako") == 0) {
        parent_node = root;
    } else {
        // 否則，尋找相應的父節點
        parent_node = find_node(root, parent_friend_name);
        if (parent_node == NULL) {
            // 找不到父節點時顯示錯誤
            char child_name[MAX_FRIEND_NAME_LEN];
            get_friend_name(child_friend_info, child_name);
            print_fail_meet(parent_friend_name, child_name);
            return;
        }
    }

    int pipe_parent_to_child[2], pipe_child_to_parent[2];
    char buffer[1024];

    // 創建兩個管道
    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        ERR_EXIT("pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        ERR_EXIT("fork");
    } else if (pid == 0) {
        // 子進程代碼
        close(pipe_parent_to_child[1]); // 關閉父進程寫的端點
        close(pipe_child_to_parent[0]); // 關閉父進程讀的端點

        // 重定向子進程的標準輸入和標準輸出到管道
        dup2(pipe_parent_to_child[0], STDIN_FILENO);
        dup2(pipe_child_to_parent[1], STDOUT_FILENO);

        // 關閉重定向後的管道端點
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);

        // 執行新的程序
        execlp("./friend", "friend", child_friend_info, (char *)NULL);
        ERR_EXIT("execlp");
    } else {
        // 父進程代碼
        close(pipe_parent_to_child[0]); // 關閉子進程讀的端點
        close(pipe_child_to_parent[1]); // 關閉子進程寫的端點

        // 寫入訊息到子進程
        write(pipe_parent_to_child[1], "Hello from parent\n", 18);

        // 讀取子進程的回應
        ssize_t n = read(pipe_child_to_parent[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Parent received: %s\n", buffer);
        }

        // 關閉管道並等待子進程結束
        close(pipe_parent_to_child[1]);
        close(pipe_child_to_parent[0]);
        waitpid(pid, NULL, 0);

	char child_name[MAX_FRIEND_NAME_LEN];
        int child_value = get_friend_value(child_friend_info);
        get_friend_name(child_friend_info, child_name);
        Node *child_node = create_node(child_name, child_value);
        child_node->pid = pid;
        add_child(parent_node, child_node);


        if (strcmp(parent_friend_name, "Not_Tako") == 0) {
            print_direct_meet(child_name);
        } else {
            print_indirect_meet(parent_friend_name, child_name);
        }
    }
}
void check(Node *root, const char *parent_friend_name) {
    Node *parent_node;

    // 如果查詢的是根節點 Not_Tako，直接使用 root
    if (strcmp(parent_friend_name, "Not_Tako") == 0) {
        parent_node = root;
    } else {
        // 否則使用 find_node 查找
        parent_node = find_node(root, parent_friend_name);
        if (parent_node == NULL) {
            printf("Not_Tako has checked, he doesn't know %s\n", parent_friend_name);
            return;
        }
    }

    // 打印節點和子樹的資訊
    print_tree(parent_node);
}

void terminate_subtree(Node *node) {
    if (node == NULL) return;

    // 關閉與當前節點子進程相關的所有管道
    close(node->pipe_parent_to_child[0]);
    close(node->pipe_parent_to_child[1]);
    close(node->pipe_child_to_parent[0]);
    close(node->pipe_child_to_parent[1]);

    // 等待當前節點的子進程結束
    if (node->pid > 0) {
        waitpid(node->pid, NULL, WNOHANG); // 等待子進程結束，避免僵屍進程
    }

    // 遞迴終止子節點 (先序遍歷)
    terminate_subtree(node->first_child);
    terminate_subtree(node->next_sibling);
}



void graduate(Node *root, const char *friend_name) {
    Node *target_node;

    // 如果 friend_name 是 Not_Tako，直接指向根節點
    if (strcmp(friend_name, "Not_Tako") == 0) {
        target_node = root;
    } else {
        // 否則使用 find_node 查找目標節點
        target_node = find_node(root, friend_name);
        if (target_node == NULL) {
            printf("Not_Tako has checked, he doesn't know %s\n", friend_name);
            return;
        }
    }

    // 執行 Check <friend_name> 顯示子樹資訊
    check(root, friend_name);

    // 終止子樹中所有節點
    terminate_subtree(target_node);

    // 如果是 Not_Tako，打印完成訊息並結束
    if (strcmp(friend_name, "Not_Tako") == 0) {
        printf("Congratulations! You've finished Not_Tako's annoying tasks!\n");
        exit(0); // 結束程式
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [friend_info]\n");
        return 0;
    }
    setvbuf(stdout, NULL, _IONBF, 0);

    strncpy(friend_info, argv[1], MAX_FRIEND_INFO_LEN);
    Node *root_node = NULL;

    if (strcmp(argv[1], root) == 0) {
        root_node = create_node("Not_Tako", 0);
        strncpy(friend_name, friend_info, MAX_FRIEND_NAME_LEN);
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';
        read_fp = stdin;
        friend_value = 100;
    }

    char command[100];
    while (fgets(command, sizeof(command), read_fp)) {
        command[strcspn(command, "\n")] = '\0';

        if (strncmp(command, "Meet", 4) == 0) {
            char *token = strtok(command, " ");
            token = strtok(NULL, " ");
            char parent_friend_name[MAX_FRIEND_NAME_LEN];
            if (token != NULL) {
                strncpy(parent_friend_name, token, MAX_FRIEND_NAME_LEN - 1);
                parent_friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';
            } else {
                fprintf(stderr, "Invalid Meet command format: %s\n", command);
                continue;
            }

            token = strtok(NULL, " ");
            char child_friend_info[MAX_FRIEND_INFO_LEN];
            if (token != NULL) {
                strncpy(child_friend_info, token, MAX_FRIEND_INFO_LEN - 1);
                child_friend_info[MAX_FRIEND_INFO_LEN - 1] = '\0';
            } else {
                fprintf(stderr, "Invalid Meet command format: %s\n", command);
                continue;
            }

            meet(root_node, parent_friend_name, child_friend_info);

        } else if (strncmp(command, "Check", 5) == 0) {
            char parent_friend_name[MAX_FRIEND_NAME_LEN];
            if (sscanf(command, "Check %s", parent_friend_name) == 1) {
                check(root_node, parent_friend_name);
            } else {
                fprintf(stderr, "Invalid Check command format: %s\n", command);
            }

        } else if (strncmp(command, "Graduate", 8) == 0) {
            char friend_name[MAX_FRIEND_NAME_LEN];
            if (sscanf(command, "Graduate %s", friend_name) == 1) {
                Node *target_node;
		
		
		if(strcmp(friend_name,"Not_Tako")==0){
			target_node = root_node;
		}else{
			target_node = find_node(root_node,friend_name);
			if(target_node ==NULL){
				print_fail_check(friend_name);
				continue;
			}
		
		}


                check(root_node, friend_name);
                terminate_subtree(target_node);
		if (strcmp(friend_name, "Not_Tako") == 0) {
                    print_final_graduate();

                    exit(0);
                }

            } else {
                fprintf(stderr, "Invalid Graduate command format: %s\n", command);
            }
        }
    }

    return 0;
}
