#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<time.h>
#include<signal.h>

char GET_STR[] = "GET";

char* getfileext(char *str){
	char* temp = (char*)malloc(100);
	int idx = 0, i = 0;
	int len = strlen(str);
	for(i = len - 1; i > 0 ; i--){
		if(str[i] == '.'){
            i += 1;
            while(i <= len){
                temp[idx] = str[i];
                idx++;
                i++;
            }
            break;
        }
	}
    if (i == 0){
        strcpy(temp,"");
    }
    return temp;
}

bool startswith(char *str, char *pre){
    int lenp = strlen(pre);
    int lens = strlen(str);
    return (lens < lenp) ? false : memcmp(str, pre, lenp) == 0;
}

void batch_terminator(int sig){
    char arr[10] = "";
    do{
        printf("Terminate batch job (Y/N)?");
        scanf("%s",arr);
    }while(strcmp(arr,"Y") != 0 && strcmp(arr,"N") != 0);
    if(strcmp(arr,"Y") == 0)
        exit(0);   
}

int main(int argc, char **argv){
    signal(SIGINT,batch_terminator);
    int port = 0;
    if(argc != 2 || (port = atoi(argv[1])) == 0 || (port < 1 || port > 65535)){
        printf("Usage: ./server [PORT]\n\nDescription:\n\t\t[PORT]\tThe port where the server is running\n");
        return 1;
    }
    
    // printf("port : %d\n",port);
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int nsocket;
    sockaddr_in socket_info;
    int addrlen = sizeof(socket_info);
    time_t t = time(NULL);

    int a = 1;

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if( server == 0 ){
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    socket_info.sin_family = AF_INET;
    socket_info.sin_port = htons(port);
    socket_info.sin_addr.s_addr = INADDR_ANY;

    if( (bind(server, (sockaddr*) &socket_info, sizeof(socket_info))) < 0 ){
        perror("Cannot bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server,3) < 0){
        perror("Cannot listen");
        exit(EXIT_FAILURE);
    }

    char uri[100] = "";
    FILE* fp = NULL;
    char* content = NULL;
    char* header = NULL;
    char temp[1000] = "";
    char* ext = NULL;
    char content_type[50] = "";
    char header_format[100] = "";
    int status_code = 0;
    char method[10] = "";
    strcpy(header_format,"HTTP/1.1 %d %s\nContent-Type: %s\nContent-Length: %d");

    printf("HTML Builder Server started at %s",asctime(localtime(&t)));
    printf("Listening on http://0.0.0.0:%d\n",port);
    puts("Press Ctrl+C to quit.");
    puts("=========================================================================");
    // printf("Waiting for connection...\n");

    while(true){
        if((nsocket = accept(server, (sockaddr*)&socket_info, (socklen_t*)&addrlen)) < 0){
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        // printf("src : %d\n",ntohs(socket_info.sin_port));

        char buffer[30000] = {0};
        int val = read(nsocket, buffer, 30000);
        // printf("%s\n",buffer);

        sscanf(buffer, "%[^ ] %[^ ] HTTP",method,uri);
        // printf("The URI : %s\n",uri);

        int size = 0;
        fp = NULL;

        if(strcmp(uri, "/") == 0){
            fp = fopen("./index.html","rb");
            strcpy(uri,"./index.html");
        } else {
            char new_uri[255] = ".";
            strcat(new_uri,uri);
            strcpy(uri,new_uri);
            fp = fopen(uri,"rb");
        }

        if (strcmp(method,"GET") != 0){
            header = (char *)malloc(100);
            status_code = 405;
            sprintf(header,"HTTP/1.1 %d Method Not Allowed\nContent-Type: text/html;\n\n<h1>405 Method Not Allowed</h1>",status_code);
        } else if ((opendir(uri)) != NULL){
            header = (char *)malloc(100);
            status_code = 403;
            sprintf(header,"HTTP/1.1 %d Forbidden\nContent-Type: text/html;\n\n<h1>403 Forbidden</h1>",status_code);
        } else if(fp == NULL){
            // printf("Not found!\n");
            header = (char *)malloc(100);
            status_code = 404;
            sprintf(header,"HTTP/1.1 %d Not Found\nContent-Type: text/html;\n\n<h1>404 Not Found</h1>",status_code);
        } else {
            ext = getfileext(uri);
            // printf("%s\n",ext);

            if(strcmp(ext,"") == 0){
                strcpy(content_type,"text/html");
            } else if (strcmp(ext,"js") == 0){
                strcpy(content_type,"application/javascript");
            } else {
                strcpy(content_type,"text/");
                strcat(content_type,ext);
                // puts(content_type);
            }

            fseeko(fp,0,SEEK_END);
            size = ftello(fp);
            rewind(fp);

            header = (char *)malloc(size + 100);
            content = (char *)malloc(size);

            int temp_size = size;
            
            while(!feof(fp)){
                if(temp_size / 512 <= 0){
                    fread(temp,1,temp_size,fp);
                    char last[temp_size + 1] = "";
                    int n = 0;
                    while(n < temp_size){
                        last[n] = temp[n];
                        n++;
                    }
                    sprintf(content,"%s%s",content,last);
                    break;
                }
                fread(temp,1,512,fp);
                temp_size -= 512;
                
                sprintf(content,"%s%s",content,temp);
            }
            
            if(size == 0){
                status_code = 204;
                sprintf(header,"HTTP/1.1 %d No Content\nContent-Type: text/html",status_code);
            } else {
                sprintf(header,header_format,200,"OK",content_type,size);
                sprintf(header,"%s\n\n%s",header,content);
                status_code = 200;
            }
        }
        write(nsocket, header, strlen(header));
        sprintf(uri - 1,"%s",uri);
        char *datetime = asctime(localtime(&t));
        datetime[strlen(datetime) - 1] = '\0';
        printf("[%s] [0.0.0.0]:%d [%d]: %s\n",datetime,ntohs(socket_info.sin_port),status_code, uri);

        close(nsocket);

        // printf("Closing socket\n");
    }
    return 0;
}