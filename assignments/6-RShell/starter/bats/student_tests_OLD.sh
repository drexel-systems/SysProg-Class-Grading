#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

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


