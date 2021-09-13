/**
 * Name(Author)       : Sontu Mistry
 * Roll no.           : 20CS60R15
 * Language : c++
 * OS       : Linux (Ubuntu)
 * Compiler : g++ (GNU)
 * N.B. No STL is used.
 * Please refer to readMe.txt file more information.
 **/

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <signal.h>
#include <string>
#include <strings.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>

#define FILE_TRANSFER "File is being transferred."
#define ERR_DUP_NAME "Error! file with same name already exist please rename and send the file again."
#define MAX 1024*8
#define PORT 8080
#define ll long long int
#define Time std::chrono::_V2::system_clock::time_point
using namespace std;
using namespace std::chrono;

ifstream fin;
ofstream fout;
bool is_interrupted = false;                                          // This global variable is required to handle kill signal.
void read_user_input(string &msg_to_send);                            // read input from the user.
int create_connection(string ip, int port, bool show_log = true);     // create a socket endpoint
void send_message(int skt_id, string ip, int port);                   // send message to the server
void send_file(int skt_id, string file_name, string ip, int port);    // used to send a file to the FTP server
void receive_file(int skt_id, string file_name, string ip, int port); // used to receive a file from the FTP server
void close_connection(int skt_id);                                    // close a socket connection
void log_error(string error_msg);                                     // log error
void log_success(string msg);                                         // log message
void sigint_handler(int signal);                                      // handle ctrl + c
bool do_file_exist(string str);                                       // checks if a file exist or not
int split(string str, string delimiter, string res_str[]);            /* Split the formatted input string. */
bool is_valid_command_with_arg(string str);                           // returns true command name is valid and command assumes a file name
bool is_valid_command(string str);                                    // returns true command name is valid and command
string to_upper(string str);                                          // convert a string to uppercase.
string rename_file(string file_name);                                 // Rename a file, if needed.
string transmission_time(int time);                                   // calcute time neede to transmit a file.

/* Driver function */
int main(int argc, char const *argv[])
{
    string ip; // will store ip address of the server.
    int port;  // will store port number of server.

    // Get hostname details
    int skt_id;
    if (argc >= 2) // read hostname as commandline argument
        ip = argv[1];
    else
        ip = "localhost";

    if (argc >= 3) // read port number as commandline argument
        port = htons(stoi(argv[2]));
    else
        port = htons(PORT);

    skt_id = create_connection(ip, port);
    send_message(skt_id, ip, port);
    close_connection(skt_id);
    return 0;
}

int create_connection(string ip, int port, bool show_log)
{
    /* This function creates a socket connection */

    // Mention right address and port
    struct hostent *server = gethostbyname(ip.c_str());
    sockaddr_in addr;
    int addrlen = sizeof(addr);
    bzero((char *)&addr, addrlen); // clear the memory
    addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);
    addr.sin_port = port;
    int option = 1;

    /* Create socket |-> (skt_id:int < 0) ==> error */
    int skt_id = socket(AF_INET, SOCK_STREAM, 0);
    if (skt_id < 0 && show_log)
        log_error("create at port " + to_string(addr.sin_port));
    else if (show_log)
        log_success("Socket created");

    /* Connect to the server */
    int conn_status = connect(skt_id, (struct sockaddr *)&addr, sizeof(addr));
    if (conn_status < 0 && show_log)
        log_error("connect ");
    else if (show_log)
        log_success("connected to server at port no. " + to_string(addr.sin_port));

    /* Return socket id */
    return skt_id;
}

bool do_file_exist(string str)
{ /* This function checks if a file (name specified by str) exist or not */
    fin.open(str.c_str());
    bool do_exist = fin.good();
    fin.close();
    return do_exist;
}

void send_message(int skt_id, string ip, int port)
{ /* This function reads user input and send control and data message accordingly to the FTP server */

    char buffer[MAX];
    while (true)
    {
        // Handle SIGINT (ctrl + c)
        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0; // or SA_RESTART
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGINT, &sa, NULL) == -1)
            exit(1);

        // Handle input
        memset(buffer, 0, MAX); // Clear the buffer.
        string command_to_send = "", file_to_send = "";
        string command = "", command_arr[10];

        // If user press only enter, force him/her to type a non empty Command.
        do
        {
            cout << flush << "\n[Client] >> Enter (<Command>/HELP/Q): " << flush;
            getline(cin, command);
        } while ((command == "") && (is_interrupted == false));

        // Split the input, if needed.
        split(command, " ", command_arr);
        command = command_arr[0];

        // If SIGINT (ctrl + c) is pressed close the connection before tarmination.
        if (is_interrupted)
            command = "QUIT";

        // If user want to exit, inform the server and exit.
        if (to_upper(command) == "QUIT" || to_upper(command) == "Q" || to_upper(command) == "EXIT")
            command = "QUIT";

        if (is_valid_command_with_arg(command))
        { /* If command is followed by a file name*/
            string file_name = command_arr[1];

            if (to_upper(command) == "STOR")
            { /** If client wants to store a file on the server.
                * Please note file will be overwritten if anyfile with same name already exist in the server.
                **/
                if (do_file_exist(file_name))
                { // If file name mentioned by the user exist then send the data.
                    command_to_send = command + " " + file_name;
                    send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send stor command to the server.
                    int recv_val = recv(skt_id, buffer, 1024, 0);                       // read if user is allowed to send the data or not.
                    if (recv_val == 0)                                                  // If no reponse / empty response is received from the server.
                    {
                        cout << "[ERROR] >> You are disconnected.\n";
                        break;
                    }
                    send_file(skt_id, file_name, ip, port);
                    cout << "[Server] >> " << buffer << endl;
                }
                else
                { // If file doesn't present in at client's end.
                    cout << "[Client] >> File name you've mentioned doesn't exist." << endl;
                }
            }

            if (to_upper(command) == "RETR")
            { // If client wants to fetch a file from the FTP server.

                command_to_send = command + " " + file_name;
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send RETR command to the server.
                int recv_val = recv(skt_id, buffer, 1024, 0);                       // IF server send the data then read it.
                if (recv_val == 0)                                                  // If no reponse / empty response is received from the server.
                {
                    cout << "[ERROR] >> You are disconnected.\n";
                    break;
                }

                if (strcmp(buffer, FILE_TRANSFER) == 0)
                {
                    receive_file(skt_id, file_name, ip, port);
                }
                cout << "[Server] >> " << buffer << endl;
            }

            if (to_upper(command) == "DELE")
            {
                command_to_send = command + " " + file_name;
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send delete command to the server.
                int recv_val = recv(skt_id, buffer, 1024, 0);                       // read confomation message from the FTP server
                if (recv_val == 0)                                                  // If no reponse / empty response is received from the server.
                {
                    cout << "[ERROR] >> You are disconnected.\n";
                    break;
                }
                else if (recv_val < 0)
                    cout << "\n[ERROR] >> Nothing received." << endl;
                else
                    cout << "[Server] >> " << buffer << endl;
            }

            if (to_upper(command) == "CODEJUD")
            {
                string extension = command_arr[2];
                command_to_send = command + " " + file_name + " " + extension;
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send codejud command to the server.

                /***************** compile ****************/
                int recv_val = recv(skt_id, buffer, MAX, 0); // read server's reply.

                // Check if any compilation message received from the server or not.
                if (recv_val == 0) // If no reponse / empty response is received from the server.
                {
                    cout << "[ERROR] >> You are disconnected.\n";
                    break;
                }
                else if (recv_val < 0)
                {
                    cout << "\n[ERROR] >> Nothing received." << endl;
                    continue;
                }

                if (strcmp(buffer, "COMPILE_ERROR") == 0 || strcmp(buffer, "ERROR!! please enter a valid program name.") == 0)
                { // Error ocurred before or during compilation
                    cout << "[Server] >> " << buffer << endl;
                }
                else if (strcmp(buffer, "COMPILE_SUCCESS") == 0)
                {
                    cout << "[Server] >> " << buffer << endl;

                    /***************** Execution **************/
                    cout << "\n[Server] >> Executing " << file_name << "..." << endl;
                    int recv_val = recv(skt_id, buffer, MAX, 0); // read server's reply.

                    // Check  Execution response received from the server or not.
                    if (recv_val == 0) // If no reponse / empty response is received from the server.
                    {
                        cout << "[ERROR] >> You are disconnected.\n";
                        break;
                    }
                    else if (recv_val < 0)
                    {
                        cout << "\n[ERROR] >> Nothing received." << endl;
                        continue;
                    }
                    else
                    {
                        cout << "\n[Server] >> " << buffer << endl;
                    }

                    /***************** Result **************/
                    recv_val = recv(skt_id, buffer, MAX, 0); // read server's reply.

                    // Check result received from the server or not.
                    if (recv_val == 0) // If no reponse / empty response is received from the server.
                    {
                        cout << "[ERROR] >> You are disconnected.\n";
                        break;
                    }
                    else if (recv_val < 0)
                    {
                        cout << "\n[ERROR] >> Nothing received." << endl;
                        continue;
                    }
                    else
                    {
                        cout << "[Server] >> " << buffer << flush;
                    }
                }
            }
        }

        else if (is_valid_command(command))
        { /* If command is valid but doesn't need a file name*/
            command_to_send = to_upper(command);
            if (command_to_send == "LIST")
            {                                                                       // If user want's to know what are the files stored at the server.
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send "LIST" command to the server.
                int recv_val = recv(skt_id, buffer, 1024 * 8, 0);                   // read file name from server.
                if (recv_val == 0)                                                  // If no reponse / empty response is received from the server.
                {
                    cout << "[ERROR] >> You are disconnected.\n";
                    break;
                }
                else if (recv_val < 0)
                    cout << "\n[ERROR] >> Nothing received." << endl;
                else
                    cout << buffer << endl;
            }

            if (command_to_send == "ABOR")
            {                                                                       // If user wants to abort the data transmission.
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0); // Send "ABOR" command to the server.
                int recv_val = recv(skt_id, buffer, 1024, 0);                       // read server's reply.
                if (recv_val == 0)                                                  // If no reponse / empty response is received from the server.
                {
                    cout << "[ERROR] >> You are disconnected.\n";
                    break;
                }
                else if (recv_val < 0)
                    cout << "\n[ERROR] >> Nothing received." << endl;
                else
                    cout << "[Server] >> " << buffer << endl;
            }

            if (is_interrupted || command_to_send == "QUIT")
            { // If User wants to quit break the loop
                send(skt_id, command_to_send.c_str(), command_to_send.length(), 0);
                break;
            }

            if (command_to_send == "HELP" || command_to_send == "'HELP'")
            {
                cout << "[Client] >> Valid commands are: \n\t 1. STOR <file_name>: to store a file in the server."
                     << "\n\t 2. RETR <file_name>: to retrive a file from the server.\n\t 3. DELE <file_name>: to delete a file from server."
                     << "\n\t 4. LIST: to see all the files present at server side. \n\t 5. ABOR: to abort a data transmission."
                     << "\n\t 6. QUIT / Q / EXIT: to exit. \n\t 7. CODEJUD <file_name> <ext>: to run and test a program. \n\t\t For example CODEJUD hello.cpp cpp. \n\t 8. HELP: to list all the valid commands." << endl;
            }
        }
        else if (!is_valid_command_with_arg(command) && !is_valid_command(command))
        { // If user doesn't enter a valid command.
            send(skt_id, command.c_str(), command.length(), 0);
            int recv_val = recv(skt_id, buffer, 1024, 0); // read server's reply.
            if (recv_val == 0)                            // If no reponse / empty response is received from the server.
            {
                cout << "[ERROR] >> You are disconnected.\n";
                break;
            }
            cout << "[Server] >> " << buffer << endl;
        }
        command == "";
    }
}

void send_file(int skt_id, string file_name, string ip, int port)
{ /* This function reads a file from hard disk and send to the FTP server. */
    int pid;
    pid = fork(); // create a child process which will do the data transmission.
    if (pid == 0) // child process
    {
        close(skt_id); // child process doesn't need command connection (socket)

        int data_sock = create_connection(ip, port + 1, false); // create a data connection.
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));                  // clear the buffer.
        fin.open(file_name.c_str(), ios::binary | ios::in); // open the file in binary format.
        fin.seekg(0, std::ios::end);                        // goto end of the file
        ll file_size = fin.tellg();                         // find out how many byte you needed to skip to reach there. i.e. file size
        fin.seekg(0, std::ios::beg);                        // goto begining of the file
        sprintf(buffer, "%lld", file_size);                 // put file size to the buffer.
        send(data_sock, buffer, sizeof(buffer), 0);         // send file size to the FTP server.

        Time start = high_resolution_clock::now();

        while (file_size > 0) // while there is data left
        {
            if (file_size >= 1024)
            { // If possible send data in chunk of 1024 byte.
                fin.read(buffer, 1024);
                send(data_sock, buffer, 1024, 0);
                file_size -= 1024;
            }
            else
            { // send the remaining file.
                fin.read(buffer, file_size);
                send(data_sock, buffer, file_size, 0);
                file_size -= file_size;
            }
        }

        for (int i = 0; i < 38; i++)
        { // Delete characters which is already printed on the consol.
            cout << '\b' << " " << '\b';
        }

        // Calculate file transmission time.
        Time end = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(end - start);
        int time = duration.count();
        string time_taken = transmission_time(time);
        cout << "[Client] >> " << file_name << " transmission successful in " + time_taken << endl;
        cout << flush << "\n[Client] >> Enter (<Command>/Help/Q): " << flush; // don't delete

        fin.close();      // close fin.
        close(data_sock); // close data connection
        exit(0);          // exit child process
    }
    return;
}

void receive_file(int skt_id, string file_name, string ip, int port)
{ /* This function receive a file send by the server and store it onto the disk. */
    int pid;
    pid = fork();
    if (pid == 0) // child process
    {
        close(skt_id); // child process doesn't need the control connection.

        int data_sock = create_connection(ip, port + 1, false); // create a data connection
        char buffer[1024];

        file_name = rename_file(file_name);                        // rename file if it already exist
        fout.open(file_name.c_str(), ios::binary | ios::out);      //open the file in binary mode.
        int recv_val = recv(data_sock, buffer, sizeof(buffer), 0); // read size of the file from the FTP server
        if (recv_val == 0)                                         // If no reponse / empty response is received from the server.
        {
            cout << "[ERROR] >> You are disconnected.\n";
            return;
        }
        ll remaining_byte = stoll(buffer); // amount of data to be read.

        Time start = high_resolution_clock::now();
        while (remaining_byte > 0) // while there is data read it
        {
            int received_byte = recv(data_sock, buffer, sizeof(buffer), 0); // receive the data.
            if (received_byte == 0)                                         // If no reponse / empty response is received from the server.
            {
                cout << "[ERROR] >> You are disconnected.\n";
                break;
            }
            fout.write(buffer, received_byte); // write the data to the file.
            remaining_byte -= received_byte;   // recalculate the remainting file size
        }

        for (int i = 0; i < 38; i++)
        { // Delete characters which is already printed on the consol.
            cout << '\b' << " " << '\b';
        }

        // Calculate file transmission time.
        Time end = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(end - start);
        int time = duration.count();
        string time_taken = transmission_time(time);

        cout << "[Client] >> " << file_name << " received successfully in " + time_taken << endl;
        cout << flush << "\n[Client] >> Enter (<Command>/Help/Q): " << flush; // don't delete

        close(data_sock); // close the data connection
        fout.close();     // close fout
        exit(0);          // exit the child process when data transmission is done.
    }
    return;
}

string transmission_time(int time)
{
    string time_taken;
    if (time >= 60 * 60)
    {
        time_taken = to_string(time / 3600) + " hr ";
        time = time % 3600;
        if (time > 0)
            time_taken += to_string(time / 60) + " min ";
        time = time % 60;
        if (time > 0)
            time_taken += to_string(time) + " sec.";
    }
    else if (time >= 60)
    {
        time_taken = to_string(time / 60) + " min ";
        time = time % 60;
        if (time > 0)
            time_taken += to_string(time) + " sec.";
    }
    else
    {
        time_taken = to_string(time) + " sec.";
    }
    return time_taken;
}

string rename_file(string file_name)
{ // This function rename the file, if needed.
    if (!do_file_exist(file_name))
    {
        return file_name;
    }
    else
    {
        string arr[50];
        int count = 1;
        int pos = 0; // where (number) is to be inserted.
        int no_of_dot = split(file_name, ".", arr);
        if (no_of_dot >= 2)
        {
            pos = no_of_dot - 2;
        }
        while (true) // find a file name which is not present.
        {
            string str = "";
            for (int i = 0; i < no_of_dot; i++)
            {
                str += arr[i];
                if (i == pos)
                {
                    str += ("(" + to_string(count) + ")");
                    count++;
                }
                if (i != no_of_dot - 1) // add the dots removed by split function.
                    str += ".";
            }
            if (!do_file_exist(str)) // If we get a unique name then break the loop.
            {
                file_name = str;
                break;
            }
        }
    }
    return file_name;
}

void close_connection(int skt_id)
{ // This function close all the open connection.
    try
    {
        close(skt_id);
        cout << "[Client] >> connection closed.\n\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

bool is_valid_command_with_arg(string str)
{ // returns true command name is valid and command assumes a file name
    str = to_upper(str);
    if (str == "RETR" || str == "STOR" || str == "DELE" || str == "CODEJUD")
        return true;
    return false;
}

bool is_valid_command(string str)
{ // returns true if command name is valid.
    str = to_upper(str);
    if (str == "LIST" || str == "ABOR" || str == "QUIT" || str == "'HELP'" || str == "HELP")
        return true;
    if (is_interrupted)
        return true;
    return false;
}

void log_error(string error_msg)
{ // Log error
    cerr << "Failed to " << error_msg << " :( \n";
    exit(1);
}

void log_success(string msg)
{
    cout << "[Client] >> " << msg << " successfully.\n";
}

void sigint_handler(int signal)
{ // This function is tiggered when user press ctrl+c
    is_interrupted = true;
    cout << "\n\t--> CTRL + C pressed. You are now disconnected." << endl;
}

int split(string str, string delimiter, string res_str[])
{
    /* Split the formatted input string. */
    string token;
    size_t pos;
    int i = 0;
    while ((pos = str.find(delimiter)) != string::npos)
    {
        token = str.substr(0, pos);
        res_str[i++] = token;
        str.erase(0, pos + delimiter.length());
    }
    res_str[i++] = str;
    return i;
}

string to_upper(string str)
{ // convert a string to upper case.
    string res = "";
    for (int i = 0; i < str.length(); i++)
    {
        if (str[i] >= 97 && str[i] <= 122)
            res += str[i] - 32;
        else
            res += str[i];
    }
    return res;
}