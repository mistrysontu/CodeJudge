Name(Author)       : Sontu Mistry
Roll no.           : 20CS60R15
Date(Last Edited)  : 07/02/2020

Language : c++
OS       : Linux (Ubuntu)
Compiler : g++ (GNU)

N.B. : store client.cpp and server.cpp in two different folder.
       in the client's folder also store the test c/cpp programs
       in the server's folder also store the input and testcase file of all test c/cpp file beforehand.

compilation command (sever)  : g++ server.cpp -o sever
compilation command (client) : g++ client.cpp -o client 

To execute:
  1. Open two terminal
  2. In 1st terminal run ./server <PORT_NO>   
            or           ./server
  3. In 2nd terminal run ./client <IP_ADDRESS_OF_SERVER> <PORT_NUMBER>
            or           ./client
  N.B.: More than one client can be run simultaneously.

Valid commands(at client's end):
    1. RETR <filename> : To retrive a file from the server if file is stored by the client (client_no will be appended at the end of the file in this case), or if the file is availabe publicly at sever side
    2. STOR <filename> : To store a file on the server. if file type is c/cpp then client_no is appended at the end of the file name at server side.
    3. DELE <filename> : To delete a file from the server.
    4. LIST : To list all the file names present at the server side. 
    5. ABOR : To abort an on going transmission of data.
    6. QUIT : To disconnect the client.
    7. CODEJUD <file_name> <ext>: to run and test a program. 
                                  For example CODEJUD hello.cpp cpp 
                                  OR 
                                  CODEJUD hello cpp
    8. CMD : show all the valid commands

ERROR Handled:
    1. If user enter any command except the valid ones then error message will be shown to the user.
    2. If user wants to store a file on the server and the file doesn't exist at the user/client side then error will be shown.
    3. If user wants to store a file on the server and a file with same name exist at the server side then it will print error message.
    4. If user wants to retrive a file which doesn't exist at the server side then an error message will be shown.
    5. If user wanst to delete a file which doesn't exist then an error message will be shown. 

disclaimer: 
  1. All code are written by the author.
  2. All code are running fine on author's computer. In case any problem please contact the author at mistrysontu@gmail.com
  3. All code are written and tested as per the information provided.

N.B. :
    1. Client is  allowed to store a file if another file with same name exist in the server side but existing file will be overwritten.
    1.1. If client stores a c/cpp file, client id will be appended at the end of the c/cpp file at the server side.
    1.2. If client A stores abc.cpp but client B wants to run abc.cpp he/she will not allowed to do so as he didn't upload that.
    1.3. CODEJUD command accept both file_name with and without extension.
    1.4. CODEJUD command return execution status and result of each testcase.
    1.5. Output at server terminal may not in order, please look at client terminal for right order. 
    2. If Client wants do retrive a file again and again file name will be autometically renamed when it is being saved at the client side. (Same thing could be done at server end.)
    3. There is no restriction on the size of the file or type of the file being send. However for very large file it may take a while to transmit the whole file.
    4. Data is being sent via a child process and data connection is closed when data transmission is over.
    5. If client want to exit, the control connection is dropped, but child process will keep sending data if there was a on going file transmission before user quit.
    6. command name are not case - sensitive however the file names are, so be careful with the case.
    7. You can use ABOR command, but it may not work as intended.
    8. Select system call is used for handling multiple client.
