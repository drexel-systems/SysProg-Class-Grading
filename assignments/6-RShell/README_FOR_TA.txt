TO run the server use the following:

./dsh -s

or, if the server is already using that address use this alternative:
./dsh -s -p 5678


To run the client use the following:

./dsh -c 127.0.0.1
(if you're using the default one)


or 

./dsh -c 127.0.0.1 -p 5678
(if that is in use)

NOTE: Make sure to update the client or server accordingly if there any changes ur adding!

FOR Multi-Threaded Mode:

Run the server: ./dsh -s -p 5678 -x
Run the Client1, (btw the client should be running in a new terminal): ./dsh -c 127.0.0.1 -p 5678
Run the Client2, (aslo in a new terminal): ./dsh -c 127.0.0.1 -p 5678


TO test the code, and some features:
Manually:

1. Run the server
2. Run the client
    2.a. Use the following command to test if that is working:
    ls
    pwd
    whoami
    hostname
    date
    cat *filename* (such as cat rsh_server.c)
    exit

GOOD TO KNOW: You can see the server, it should print what command it recieved from clients (Just to make your life easier :))

TO run the test cases, cd to bats, then run the following test file:
./assignment_tests.sh        (this is build in and for the assignemnt specific)

NOTE: this might be a bit slow as it will work with client and server sockets.
(just in case make sure to first "make" and copy the dsh into bats, but I will have it done for you, dw)


