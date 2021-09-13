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
#include <dirent.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#define PORT 8080
#define MAX 1000
#define MAX_INT 2147483647
#define MIN -2147483648
#define ll long long int
#define FILE_TRANSFER "File is being transferred."
#define ERR_DUP_NAME "Error! file with same name already exist please rename and send the file again."
#define Time std::chrono::_V2::system_clock::time_point
using namespace std::chrono;
using namespace std;

ofstream fout, fout1;                  // used to write file
ifstream fin;                          // used to read file
ifstream inp_file_obj1, inp_file_obj2; // used to compare two file

int create_connection(int port, bool show_msg = true);                  // Create a new socket.
int split(string str, string delimiter, string res_str[]);              /* Split the formatted input string. */
string get_filename_of_client(string file_name, int client_id);         // append client_no after file name
string get_filename_wo_ext(string file_name);                           // returns file name without extension
string get_obj_file_name(string file_name);                             // returns object file name
string get_filename_with_cid(string file_name, int client_id);          // returns file name with client_no appended at the end
string get_inp_test_file_name(string file_name);                        // returns input or testcase file name, given c/cpp program name.
string compare_test_cases(string, string, string, bool);                /* This function compare testcase output and output produce the user program and find out if they are same or not */
string to_upper(string str);                                            // convert a string to uppercase.
string get_all_file_name();                                             // Returns name of all file present in the server end.
string float_to_string(float a_value, const int precision = 2);         // convert flaot number to a string with precision 2.
void CODEJUD(string file_name, string file_type, int client_id);        // Evaluate the code mentioned by the user.
void handle_request(int skt_id, int data_sock);                         // Handle client's request.
void close_connection(int skt_id);                                      // Closes open connection and open files
void log_error(string error_msg);                                       // Log message
void log_success(string msg);                                           // Log message
void send_file(int data_sock, int client_id, string fname, int skt_id); // used to send a file from the FTP server to the user
void receive_file(int data_sock, int c_id, string fname, int skt_id);   // used to receive a file from the client
void serve_client(int, sockaddr_in &, fd_set &, int, int);              // Serve client requests.
bool is_valid_command_with_arg(string str);                             // returns true command name is valid and command assumes a file name
bool is_valid_command(string str);                                      // returns true command name is valid and command
bool do_file_exist(string str);                                         // checks if a file exist or not
bool is_valid_program_name(string file_name, string file_type);         /* This function checks if mentioned file exist or not. If yes then if also check extension type */

int main(int argc, char const *argv[])
{
    // Variable needed to store socket details
    int skt_id, data_sock;

    int port_no;
    // read port number as commandline argument, if available
    if (argc >= 2)
        port_no = htons(stoi(argv[1]));
    else
        port_no = htons(PORT);

    skt_id = create_connection(port_no);               // create control connection (persistant)
    data_sock = create_connection(port_no + 1, false); // create data connection (non-persistant)
    handle_request(skt_id, data_sock);
    close_connection(skt_id);
    return 0;
}

int create_connection(int port_no, bool show_msg)
{
    /* This function creates a socket connection */
    int option = 1;
    sockaddr_in addr;
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port_no;

    // Find host name
    struct hostent *server;
    server = gethostbyname("localhost");

    // Set server address
    bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr,
          server->h_length);

    // Create socket |-> (skt_id:int == 0) ==> error
    int skt_id = socket(AF_INET, SOCK_STREAM, 0);
    if (skt_id == 0 && show_msg)
        log_error("create socket");
    else if (show_msg)
        log_success("Socket created");

    // Initialize socket |-> (skt_init_id:int == 0) ==> success.
    int skt_init_id;
    skt_init_id = setsockopt(skt_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option));
    if (skt_init_id == 1 && show_msg)
        log_error("socket initialization");
    else if (show_msg)
        log_success("Socket initialized");

    // Bind Socket
    int bind_id;
    bind_id = bind(skt_id, (struct sockaddr *)&addr, sizeof(addr));
    if (bind_id < 0 && show_msg)
        log_error("socket binding");
    else if (show_msg)
        log_success("sock binded at port " + to_string(addr.sin_port));

    //Listen to a request
    int can_listen;
    can_listen = listen(skt_id, 10);
    if (can_listen < 0 && show_msg)
        log_error("listen");
    else if (show_msg)
        log_success("listening");

    //Return Socket id.
    return skt_id;
}

void handle_request(int skt_id, int data_sock)
{
    /**
     * This function accepts requests from multiple client.
     * It handles multiple clients using select system call
     **/
    int client_id;
    sockaddr_in client_addr;

    fd_set current_FD;
    // Initialize all set of socket
    FD_ZERO(&current_FD);
    FD_SET(skt_id, &current_FD);
    while (true)
    {
        // Copy the details of current_FD into an other socket (ready_FD), as select system call is destructive in nature.
        fd_set ready_FD = current_FD;

        int skt_count = select(FD_SETSIZE, &ready_FD, nullptr, nullptr, nullptr);
        if (skt_count < 0)
        {
            cerr << "select error" << endl;
            exit(1);
        }

        // If no error occur while using select, scan all the FD and check which one has some data to send.
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_FD)) // check which FD is ready
            {
                if (i == skt_id) // If FD is server, then accecpt new connection.
                {
                    // There is a new client waiting
                    int client_addr_len = sizeof(client_addr);
                    client_id = accept(skt_id, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

                    if (client_id >= 0) // If a new connection is accepted.
                    {
                        cout << "\n\t    [+] User " << client_id << "(";
                        cout << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ") connected.\n";
                        // Add the new connection to the connected clients list
                        FD_SET(client_id, &current_FD);
                    }
                }
                else // if any client has somthing to send.
                    serve_client(i, client_addr, current_FD, data_sock, skt_id);
            } // end if
        }     // end for

    } // End of while
}

void serve_client(int client_id, sockaddr_in &client_addr, fd_set &current_FD, int data_sock, int skt_id)
{
    /**
     * This function read a FTP command from the users and return appropiate result to the users.
     **/
    char buffer[1024] = {0};
    string msg_to_send = "", data_to_send = "";
    memset(buffer, 0, sizeof(buffer));              // clear the buffer
    int valread = recv(client_id, buffer, 1024, 0); // control line
    string temp, temp_arr[10];
    split(buffer, " ", temp_arr);

    // Execute the command
    string command = to_upper(temp_arr[0]);
    cout << "\n[Client " << client_id << "] >> " << command << " ";

    int flag = 0;
    if (is_valid_command_with_arg(command))
    { // If command is followed by a file name

        string file_name = temp_arr[1];
        cout << file_name << endl;

        if (command == "RETR")
        { // client asked for a file.

            if (!do_file_exist(file_name))
            {
                string temp_file = get_filename_with_cid(file_name, client_id);
                if (do_file_exist(temp_file))
                {
                    file_name = temp_file;
                }
            }

            if (do_file_exist(file_name))
            { // If file exist, send the file to the client.
                msg_to_send = FILE_TRANSFER;
                send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0); // notify file exist and be ready to receive the file.
                send_file(data_sock, client_id, file_name, skt_id);            // Send the requested file.
                cout << "[Server] >> " << msg_to_send << endl;
            }
            else
            { // File doesn't exist
                msg_to_send = "Sorry file specified doesn't exit.";
                send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0);
                cout << "[Server] >> " << msg_to_send << endl;
            }
        }
        else if (command == "STOR")
        { /* client wants to store a file.
           * Please not if file with same name already exist then it will be overwritten.
           **/
            msg_to_send = "File is being saved";
            send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0); // tell client to send the file
            receive_file(data_sock, client_id, file_name, skt_id);         // receive the file.
            cout << "[Server] >> " << msg_to_send << endl;
        }
        else if (command == "DELE")
        { // client wants to delete a file.
            if (do_file_exist(file_name) || do_file_exist(get_filename_with_cid(file_name, client_id)))
            { // if file exist delete the file
                remove(file_name.c_str());
                remove(get_filename_with_cid(file_name, client_id).c_str());
                msg_to_send = file_name + " is successfully deleted.";
                send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0);
                cout << "[Server] >> " << msg_to_send << endl;
            }
            else
            { // send error message to the user
                msg_to_send = file_name + " can't be deleted as it doesn't exist.\n";
                send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0);
                cout << "[Server] >> " << msg_to_send << endl;
            }
        }
        else if (command == "CODEJUD")
        {
            string file_type = temp_arr[2];
            CODEJUD(file_name, file_type, client_id);
        }
    }

    else if (is_valid_command(command))
    { // if command name is valid but it don't need any filename
        cout << endl;
        if (command == "LIST")
        { // user wants to list all the file
            msg_to_send = get_all_file_name();
            send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0); // send all file name.
            cout << "[Server] >> List of file name send to the user/client." << endl;
        }
        else if (command == "ABOR")
        { // if user wants to abort the transmission.
            msg_to_send = "You requested for \"ABORT\". it's under development.";
            cout << msg_to_send << endl;
            send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0);
        }
        else if (command == "QUIT")
        { // If user wants to disconnect
            msg_to_send = "Farewell User.";
            send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0); // send farewell message
            cout << "\t    [-] User " << client_id << " disconnected.\n";
            FD_CLR(client_id, &current_FD); // clean file descriptor entry.
            close(client_id);               // close the (client) socket
        }
    }
    else
    { // For any invalid case
        msg_to_send = "Invalid command. Enter 'HELP' to see the valid commands.";
        send(client_id, msg_to_send.c_str(), msg_to_send.length(), 0);
        cout << "\n[Client " << client_id << "] >> " << command << endl;
        cout << "\[Server] >> Invalid command." << endl;
    }
}

void send_file(int data_sock, int client_id, string file_name, int skt_id)
{ /* This function reads a file from hard disk of FTP server and send to the FTP client. */

    int pid = fork(); // create a child process
    if (pid == 0)     // child process
    {
        close(skt_id);    // child process doesn't need server's control connection (socket)
        close(client_id); // child process doesn't need client's control connection (socket)

        sockaddr_in client_data_addr;
        int client_data_addr_len = sizeof(client_data_addr);
        int client_data_sock = accept(data_sock, (struct sockaddr *)&client_data_addr, (socklen_t *)&client_data_addr_len);
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));                  // clear the buffer
        fin.open(file_name.c_str(), ios::binary | ios::in); // open the file in binary mode
        fin.seekg(0, std::ios::end);                        // goto end of the file
        ll file_size = fin.tellg();                         // this return size of the file in bytes
        fin.seekg(0, std::ios::beg);                        // goto begining of the file.
        sprintf(buffer, "%lld", file_size);                 // put file size to the buffer.
        send(client_data_sock, buffer, sizeof(buffer), 0);  // send file size to the FTP server.

        while (file_size > 0) // file there is still data to read
        {
            if (file_size < 1024)
            { // if file size is smaller than 1024 byte chank then send that much data.
                fin.read(buffer, file_size);
                send(client_data_sock, buffer, file_size, 0);
                file_size -= file_size;
            }
            else
            { // send data of 1024 byte chunk
                fin.read(buffer, 1024);
                send(client_data_sock, buffer, 1024, 0);
                file_size -= 1024;
            }
        }

        cout << "[Server] >> " + file_name + " transmission successful." << endl;
        fin.close();             // close fin.
        close(client_data_sock); // close client's data socket.
        close(data_sock);        // close data socket of server.
        exit(0);                 // exit the child process when data transmission is done.
    }
    return;
}

void receive_file(int data_sock, int client_id, string file_name, int skt_id)
{ /* This function receive a file send by the FTP client and store it onto the disk at server end. */

    int pid = fork(); // create a child process

    if (pid == 0) // child process
    {
        // If file is a c/cpp file then rename the file and then save it.
        string arr[10];
        int no_of_dot = split(file_name, ".", arr);
        if (arr[no_of_dot - 1] == "c" || arr[no_of_dot - 1] == "cpp")
            file_name = get_filename_with_cid(file_name, client_id);

        close(skt_id);    // child process doesn't need server's control connection (socket)
        close(client_id); // child process doesn't need client's control connection (socket)

        sockaddr_in client_data_addr;
        int client_data_addr_len = sizeof(client_data_addr);
        cout << "\t    receiveing file... " << endl;
        int client_data_skt = accept(data_sock, (struct sockaddr *)&client_data_addr, (socklen_t *)&client_data_addr_len);
        char buffer[1024];

        fout.open(file_name.c_str(), ios::binary | ios::out); //open the file in binary mode.
        recv(client_data_skt, buffer, sizeof(buffer), 0);     // read size of the file from the client
        ll remaining_byte = stoll(buffer);                    // amount of data to be read.

        while (remaining_byte > 0) // while there is data read it
        {
            int received_byte = recv(client_data_skt, buffer, sizeof(buffer), 0); // receive the data.
            fout.write(buffer, received_byte);                                    // write the data to the file.
            remaining_byte -= received_byte;                                      // recalculate the remainting file size
        }

        cout << "[Server] >> " << file_name << " received successfully." << endl;
        close(client_data_skt); // close client connection
        fout.close();
        exit(0); // exit the child process when data transmission is done.
    }
    return;
}

string compare_test_cases(string our_file, string original_output, string fname, bool is_single_testcase)
{
    /* This function compare testcase output and output produce the user program and find out if they are same or not */
    bool do_all_testcase_passed = true; // This is a flag variable, used to find if all testcase passed or not.

    string result_file = "result_" + fname + ".txt"; // Stores the output file name
    fout.open(result_file.c_str());                  // used to store result of comparison.

    inp_file_obj1.open(original_output); // open testcase file
    if (!inp_file_obj1.good())           // If no testcase file present in the server.
    {
        cout << "ERROR!! No testcase file present." << endl;
        fout << "[Server] >> ERROR!! No testcase file present.\n";
        return "ERROR!! No testcase file present.\n";
    }
    inp_file_obj2.open(our_file); // open program output
    string expected_val, program_output;
    int count = 1;

    while (!inp_file_obj1.eof() && !inp_file_obj2.eof()) // Until one of the file reaches the end of file compare both of them.
    {
        getline(inp_file_obj1, expected_val);   // read one line from testcase file
        getline(inp_file_obj2, program_output); // read one line from program output file.

        // Remove any newline character present at the end of testcase (line).
        if (expected_val[expected_val.length() - 1] == '\n' || expected_val[expected_val.length() - 1] == '\r')
            expected_val = expected_val.substr(0, expected_val.size() - 1);

        // Remove any newline character present at the end of output (line).
        if (program_output[program_output.length() - 1] == '\n' || program_output[program_output.length() - 1] == '\r')
            program_output = program_output.substr(0, program_output.size() - 1);

        if (expected_val == program_output)
            fout << "Testcase" << count++ << " accepted." << endl;

        else
        {
            fout << "Testcase" << count++ << " failed!!" << endl;
            do_all_testcase_passed = false;
        }
    }

    while (!inp_file_obj1.eof()) // If tasecase file doesn't end but program output ends
    {
        getline(inp_file_obj1, expected_val);
        fout << "Testcase" << count++ << " failed!!" << endl;
        do_all_testcase_passed = false;
    }

    if (is_single_testcase)
    {
        /**
         * It is an extra feature.
         * If a program don't need input then we assume we will have only one testcase.
         **/
        remove(result_file.c_str());
        fout.close();
        fout.open(result_file.c_str());
        if (do_all_testcase_passed)
            fout << "Testcase1 accepted." << endl;

        else
            fout << "Testcase failed!!" << endl;
    }

    // close all the opened file.
    fout.close();
    inp_file_obj1.close();
    inp_file_obj2.close();

    // Print and return result
    cout << "\n[Server] >> ";
    string result = "";
    fin.open(result_file.c_str());
    while (!fin.eof())
    {
        string temp;
        getline(fin, temp);
        cout << temp << "\n\t    ";
        result += (temp + "\n\t    ");
    }
    fin.close();

    return result;
}

void CODEJUD(string file_name, string file_type, int client_id)
{
    /**
     * This function evaluate a program submitted by the used. This function do the followings
     * 1. Complie the c/cpp file
     * 2. Execute the object file
     * 3. Match the output
     * 4. It also handle almost all possible errors.
     **/
    int pid = fork();
    if (pid == 0)
    {
        /****************************** Pre-compilation phase *******************************/
        bool reached_end_of_file = true; // Use to track if a file reach the end of an input file or not
        bool is_single_testcase = false; // Use to track if for a program if we have only one testcase or not.

        // Do pre-prossesing
        string arr[10];
        int no_of_dot = split(file_name, ".", arr);
        if (no_of_dot == 1)
        {
            if (file_type == "c" || file_type == "cpp")
            { // If file extension isn't mentioned with file name then append file extension with file name.
                file_name = file_name + "." + file_type;
                no_of_dot++;
                arr[no_of_dot - 1] = file_type;
            }
        }

        if (arr[no_of_dot - 1] == "c" || arr[no_of_dot - 1] == "cpp")
        { // if mentioned file is a c or cpp file then find correponding file name (as file name is client specific)
            file_name = get_filename_of_client(file_name, client_id);
        }

        // Check if program name mentioned by the user present in at server end or not.
        if (is_valid_program_name(file_name, file_type))
        {
            string command = "";
            string obj_file_name = get_obj_file_name(file_name);
            string fname = get_filename_wo_ext(file_name);
            string output_file_name = "output_" + fname + ".txt";
            string temp_output_file_name = "temp_output_" + fname + ".txt";
            string temp_input_file_name = "temp_input_" + fname + ".txt";
            string err_file_name = "err_" + fname + ".txt";

            // Determine the compiler based on the file type
            if (file_type == "cpp")
                command = "g++ " + file_name + " -o " + obj_file_name;
            else if (file_type == "c")
                command = "gcc " + file_name + " -o " + obj_file_name;

            /****************************** Compilation Phase ********************************/
            string compile_msg = "";
            string cmd = command + " 2> debug_file.txt";
            int val = system(cmd.c_str());

            if (val > 0)
            { // If there is some error during compilation
                compile_msg = "COMPILE_ERROR";
                cout << "[Server] >> " << compile_msg << endl;
                send(client_id, compile_msg.c_str(), compile_msg.length(), 0);
                return;
            }
            else if (val == 0)
            { // If no compilation error.
                compile_msg = "COMPILE_SUCCESS";
                cout << "[Server] >> " << compile_msg << endl;
                send(client_id, compile_msg.c_str(), compile_msg.length(), 0);
            }
            else if (val < 0)
            { // If system call error.
                cout << "[Server] >> System call Error!!" << endl;
                compile_msg = "Unexpected error ocurred please try again.";
                send(client_id, compile_msg.c_str(), compile_msg.length(), 0);
            }

            /********************************* Execution Phase ********************************/

            string execution_msg = "";
            remove(output_file_name.c_str());       // remove output file if already present
            fout1.open(output_file_name, ios::app); // open output file.

            // Open the input file using ifstream object for the particular c/cpp file to be executed.
            string input_file = get_inp_test_file_name("input_" + fname + ".txt");
            fin.open(input_file.c_str());          // open input file
            bool do_input_file_exist = fin.good(); // check if input file present or not (i.e. if a program needs input or not)

            cout << "\n[Server] >> ";
            while (reached_end_of_file) // If file pointer doesn't reach the end of the input file
            {
                string one_testcase;
                getline(fin, one_testcase);              // Read one line from the input file.
                fout.open(temp_input_file_name.c_str()); // open an other temporty file.
                fout << one_testcase << endl;            // store that line into another temporary file.

                // Use that tempory file to execute the c/cpp file.
                string execution_command;
                if (do_input_file_exist) // If program needs input
                {
                    execution_command = "timeout --preserve-status 1 ./" + obj_file_name + " < " + temp_input_file_name.c_str() + " > " + temp_output_file_name.c_str() + " 2> " + err_file_name.c_str();
                    if (fin.eof()) // If file pointer reach the end of file
                        reached_end_of_file = false;
                }
                else // If program doesn't need any input
                {
                    execution_command = "timeout --preserve-status 1 ./" + obj_file_name + " > " + temp_output_file_name.c_str() + " 2> err_" + fname + ".txt";
                    is_single_testcase = true;
                    reached_end_of_file = false;
                }

                // Measure the execution time.
                Time start = high_resolution_clock::now();
                int execution_return_value = system(execution_command.c_str());
                Time end = high_resolution_clock::now();
                auto duration = duration_cast<seconds>(end - start);

                if (execution_return_value == 0) // Execution successful
                {
                    inp_file_obj1.open(temp_output_file_name);
                    string result;
                    while (!inp_file_obj1.eof())
                    {
                        getline(inp_file_obj1, result);
                        if (result != "")
                            fout1 << result << endl;
                    }
                    inp_file_obj1.close();
                    cout << "EXECUTION_SUCCESS\n\t    ";
                    execution_msg += "EXECUTION_SUCCESS\n\t    ";
                }
                else if (execution_return_value == 36608 || duration.count() >= 1.0)
                {
                    fout1.seekp(0, std::ios::end);
                    fout1 << "TLE\n";
                    execution_msg += "TIME_LIMIT_EXCEEDED\n\t    ";
                    cout << "TIME_LIMIT_EXCEEDED\n\t    ";
                }
                else
                {
                    fout1.seekp(0, std::ios::end);
                    fout1 << "RUN_ERROR\n";
                    execution_msg += "RUN_ERROR\n\t    ";
                    cout << "RUN_ERROR\n\t    ";
                }
                fout.close(); // close tempory file
            }

            fout1.close(); // close the output file.
            fin.close();   // close input file.

            // Notify user about execution status.
            send(client_id, execution_msg.c_str(), execution_msg.length(), 0);

            /******************************** Matching Phase ***************************/
            string testcase_file = get_inp_test_file_name("testcase_" + fname + ".txt");
            string result = compare_test_cases(output_file_name, testcase_file, fname, is_single_testcase);
            send(client_id, result.c_str(), result.length(), 0);

            /********************************* Clean Up Phase **************************/
            // Delete unnecessary files
            remove(temp_input_file_name.c_str());
            remove(temp_output_file_name.c_str());
            remove(obj_file_name.c_str());
            remove(output_file_name.c_str());
            remove(err_file_name.c_str());
            remove("debug_file.txt");
        }
        else
        { // If program name mentioned is not valid.
            string err_msg = "ERROR!! please enter a valid program name.";
            cout << "[Server] >> Invalid program name mentioned." << endl;
            send(client_id, err_msg.c_str(), err_msg.length(), 0);
        }
        close(client_id);
        exit(0);
    }
}

bool do_file_exist(string str)
{ // This file returns true if file mentioned by 'str' present in the directory.
    fin.open(str.c_str());
    bool do_exits = fin.good();
    fin.close();
    return do_exits;
}

string get_all_file_name()
{ /* This function returns name of all file present at the FTP server. */
    string files = "[Server] >> List of files:";
    DIR *dr;
    struct dirent *en;
    int count = 0;
    dr = opendir("./"); //open directory
    if (dr)
    {
        while ((en = readdir(dr)) != NULL)
        {

            if (!(string(en->d_name) == "." || string(en->d_name) == ".."))
            {
                fin.open(en->d_name, ios::binary | ios::in); // open the file in binary mode
                fin.seekg(0, std::ios::end);                 // goto end of the file
                ll file_size = fin.tellg();                  // this return size of the file in bytes
                fin.seekg(0, std::ios::beg);                 // goto begining of the file.
                fin.close();
                string filesize;
                if (file_size > 1024 * 1024 * 1024)
                {
                    filesize = float_to_string(file_size / (1024 * 1024 * 1024 * 1.0)) + " GB";
                }
                else if (file_size > 1024 * 1024)
                {
                    filesize = float_to_string(file_size / (1024 * 1024 * 1.0)) + " MB";
                }
                else if (file_size > 1024)
                {
                    filesize = float_to_string(file_size / (1024 * 1.0)) + " KB";
                }
                else
                {
                    filesize = to_string(file_size) + " Byte";
                }
                count++;
                files += ("\n\t     (" + to_string(count) + ") " + string(en->d_name) + " (" + filesize + ").");
            }
        }
        closedir(dr); //close directory
    }
    return files;
}

void close_connection(int skt_id)
{ // This function close all the open connection.
    try
    {
        fout.close();
        fin.close();
        close(skt_id);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void log_error(string error_msg)
{ // Log error
    cerr << "[Server] Failed to " << error_msg << " :( \n";
    exit(1);
}

void log_success(string msg)
{ // log message
    cout << "[Server] >> " << msg << " successfully.\n";
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
    if (str == "LIST" || str == "ABOR" || str == "QUIT")
        return true;
    return false;
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

string float_to_string(float a_value, const int n)
{ // convert float to string with precision 'n=2'
    ostringstream out;
    out.precision(n);
    out << fixed << a_value;
    return out.str();
}

bool is_valid_program_name(string file_name, string file_type)
{
    /* This function checks if mentioned file exist or not. If yes then if also check extension type */
    if (!(file_type == "c" || file_type == "cpp"))
    { // If file type isn't c or cpp return false.
        return false;
    }

    if (do_file_exist(file_name))
    { // If file fname specified present in the server.
        bool do_file_type_matched = false;
        string arr[50];
        int no_of_dot = split(file_name, ".", arr);
        if (arr[no_of_dot - 1] == file_type)
        { // if file extension is same as file_type
            do_file_type_matched = true;
        }
        return do_file_type_matched; // If file extension and file_type doesn't matched.
    }

    return false; // if file doesn't exist.
}

string get_filename_wo_ext(string file_name)
{
    /* This function returns filename removing the extension. */
    string arr[50];
    int no_of_dot = split(file_name, ".", arr);
    string str = "";
    for (int i = 0; i <= no_of_dot - 2; i++)
    {
        str += arr[i];
        if (i != no_of_dot - 2)
            str += arr[i] + ".";
    }
    return str;
}

string get_filename_with_cid(string file_name, int client_id)
{ // returns file name with client_no appended at the end
    string arr[50];
    int no_of_dot = split(file_name, ".", arr);
    string str = "";
    for (int i = 0; i <= no_of_dot - 1; i++)
    {
        str += arr[i];
        if (i < no_of_dot - 2)
            str += arr[i] + ".";
        else if (i == no_of_dot - 2)
            str += ("__" + to_string(client_id) + ".");
    }
    return str;
}

string get_obj_file_name(string file_name)
{ /* This function return an object file name */
    return get_filename_wo_ext(file_name) + ".out";
}

string get_filename_of_client(string file_name, int client_id)
{ // append client_no after file name
    string arr[10];
    int no_of_split = split(file_name, "__", arr);
    string temp_filename = get_filename_with_cid(file_name, client_id);

    if (no_of_split > 1)
    { // if client_no already appended at the end of the file return file_name
        return file_name;
    }
    else if (do_file_exist(temp_filename))
    { // If file_name is valid, return it.
        file_name = temp_filename;
    }
    else if (do_file_exist(file_name))
    { // If file without client_no exist then create a file with client_no appended at the end and copy the content to it.
        fin.open(file_name, ios::binary);
        fout.open(temp_filename, ios::binary);
        while (!fin.eof())
        { // copy the content
            string temp;
            getline(fin, temp);
            fout << temp << endl;
        }
        fin.close();
        fout.close();
        file_name = temp_filename;
    }
    return file_name;
}

string get_inp_test_file_name(string file_name)
{ /* returns input or testcase file name, given c/cpp program name. */

    string arr[10];
    string arr1[10];
    string temp = "";
    int no_of_split = split(file_name, "__", arr);
    if (no_of_split > 1)
    { // if client number is already appended after c/cpp file name,remove it
        for (int i = 0; i <= no_of_split - 2; i++)
        {
            temp += (arr[i]);
            if (i != no_of_split - 2)
            {
                temp += "__";
            }
        }
        int no_of_dot = split(arr[no_of_split - 1], ".", arr1);
        for (int i = 1; i <= no_of_dot - 1; i++)
        {
            temp += ("." + arr1[i]);
        }
    }
    else
    {
        return file_name;
    }
    return temp;
}
