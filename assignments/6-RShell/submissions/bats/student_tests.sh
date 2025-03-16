#!/usr/bin/env bats

# File: student_tests.sh
# Test suite for dsh remote shell

SERVER_PORT=5678
SERVER_IP="127.0.0.1"

setup() {
    # Start the server in the background before each test
    ./dsh -s -p $SERVER_PORT -x &  
    sleep 1  # Allow time for the server to start
}

teardown() {
    # Stop the server after each test
    echo "stop-server" | ./dsh -c $SERVER_IP -p $SERVER_PORT
    sleep 2  # Allow time for shutdown
}

@test "Verify ls command runs correctly" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Handle empty pipe segment gracefully" {
    run "./dsh" <<EOF                
echo "test" | | echo "error"
EOF

    # Strip all whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Should contain a warning or error message
    [[ "$stripped_output" == *"warning"* ]] || [[ "$stripped_output" == *"error"* ]] 

    [ "$status" -eq 0 ]
}

@test "Exceeding maximum allowed pipes" {
    # Create a long pipeline
    long_pipe="echo sample"
    for i in {1..18}; do
        long_pipe="$long_pipe | grep sample"
    done
    
    run "./dsh" <<EOF                
$long_pipe
EOF

    # Strip whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Should contain error about too many commands
    [[ "$stripped_output" == *"error"* && "$stripped_output" == *"pipe"* ]] 

    [ "$status" -eq 0 ]
}

@test "Exit after pipe execution" {
    run "./dsh" <<EOF                
echo "done" | grep done
exit
EOF

    # Strip all whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output with whitespace removed
    expected_output="donedsh3>dsh3>cmdloopreturned-7"

    echo "Captured stdout:" 
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    [[ "$stripped_output" == *"done"* ]]

    [ "$status" -eq 0 ]
}


# New Test Cases 
@test "Pipe with special characters in text" {
    # Create a test file with special characters
    echo 'Hello | World' > special_chars_test.txt
    
    run "./dsh" <<EOF                
cat special_chars_test.txt | grep Hello
EOF

    # Clean up
    rm -f special_chars_test.txt

    # Strip whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Check for "Hello" in the output
    [[ "$stripped_output" == *"Hello"* ]]

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Process large dataset through pipe" {
    # Create a temporary file with 100 lines
    for i in {1..100}; do
        echo "Line $i" >> data.txt
    done
    
    run "./dsh" <<EOF                
cat data.txt | wc -l
EOF

    # Clean up
    rm -f data.txt

    # Strip whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output is "100"
    [[ "$stripped_output" == *"100"* ]]

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Error message propagation through pipes" {
    run "./dsh" <<EOF                
ls /invalid_path | grep test
EOF

    # Error should appear for the first command but not crash the shell
    [[ "$output" == *"No such file"* || "$output" == *"cannot access"* || "$output" == *"not found"* ]]

    # Assertions - shell should exit successfully
    [ "$status" -eq 0 ]
}

@test "Command not found during pipe execution" {
    run "./dsh" <<EOF                
echo "example" | fakecommand
EOF

    # Should output error for the second command but not crash the shell
    [[ "$output" == *"not found"* || "$output" == *"command not found"* || "$output" == *"No such file"* ]]

    # Assertions - shell should exit successfully
    [ "$status" -eq 0 ]
}

@test "Redirect stderr through pipe" {
    run "./dsh" <<EOF                
ls /nonexistent_directory 2>&1 | grep "No such file"
EOF

    # Should contain error message from ls
    [[ "$output" == *"No such file"* || "$output" == *"cannot access"* || "$output" == *"not found"* ]]

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Three-stage pipe operation" {
    # Create a test file
    echo -e "item1\nitem2\nitem3\nitem4" > items.txt
    
    run "./dsh" <<EOF                
cat items.txt | grep item | wc -l
EOF

    # Clean up
    rm -f items.txt

    # Strip whitespace from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output is "4"
    [[ "$stripped_output" == *"4"* ]]

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Chaining pipes consecutively" {
    run "./dsh" <<EOF
ls | grep dsh
echo "another test" | wc -w
EOF

    # Should contain "dsh" from the first command
    [[ "$output" == *"dsh"* ]]
    
    # Should contain "2" from the second command
    [[ "$output" == *"2"* ]]

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Pipe with simulated process substitution behavior" {
    # Create two test files
    echo -e "apple\nbanana\ncherry" > fruits1.txt
    echo -e "banana\ncherry\ndate" > fruits2.txt
    
    run "./dsh" <<EOF
cat fruits1.txt | grep -f fruits2.txt
EOF

    # Clean up temp files
    rm -f fruits1.txt fruits2.txt

    # Should output "banana" and "cherry"
    [[ "$output" == *"banana"* ]]
    [[ "$output" == *"cherry"* ]]
    
    # Assertions
    [ "$status" -eq 0 ]
}

@test "Advanced filter chain with pipes" {
    run "./dsh" <<EOF
cat /etc/passwd | grep a | grep e | grep i | wc -l
EOF

    # Not checking the exact number here, as it can vary
    # Just ensuring the command runs without errors

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Check if ls runs without errors" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
    [ -n "$output" ]  # Ensure output is not empty
}

@test "Check if pwd runs without errors" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"/home"* ]]  # Ensure it prints a valid directory path
}

@test "Check whoami returns the correct user" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
whoami
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"$(whoami)"* ]]  # Fix: Use `whoami` directly
}

@test "Check a simple echo command" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
echo Hello, Remote Shell!
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Hello, Remote Shell!"* ]]
}

@test "Check command chaining with &&" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
mkdir test_dir && cd test_dir && pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"test_dir"* ]]
}

@test "Check background process execution" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
sleep 2 &
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Check file creation with touch" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
touch my_test_file
ls
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"my_test_file"* ]]  # Ensure the file is listed
}


@test "Check server exit command" {
    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
stop-server
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Server stopping."* ]]
}

@test "Check multi-client handling" {
    ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF &
ls
exit
EOF
    wait  # Ensure first client exits before running the second

    run ./dsh -c $SERVER_IP -p $SERVER_PORT <<EOF
echo Multi-client test
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Multi-client test"* ]]
}
