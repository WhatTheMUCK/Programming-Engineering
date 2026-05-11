#!/bin/bash

# Load Testing Script for Email Service
# Comprehensive performance testing with monitoring

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BASE_URL="http://localhost:8080"
RESULTS_DIR="./load_test_results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Create results directory
mkdir -p "$RESULTS_DIR"

echo -e "${GREEN}=== Email Service Load Testing ===${NC}"
echo "Timestamp: $TIMESTAMP"
echo "Results directory: $RESULTS_DIR"
echo "Base URL: $BASE_URL"
echo ""

# Helper function to run load test
run_load_test() {
    local test_name=$1
    local endpoint=$2
    local method=$3
    local data=$4
    local requests=$5
    local concurrency=$6
    
    echo -e "${YELLOW}Running: $test_name${NC}"
    echo "Endpoint: $method $endpoint"
    echo "Requests: $requests, Concurrency: $concurrency"
    
    local output_file="$RESULTS_DIR/${test_name}_${TIMESTAMP}.txt"
    
    if [ "$method" = "POST" ]; then
        ab -n "$requests" -c "$concurrency" -p "$data" -T "application/json" \
           -g "$RESULTS_DIR/${test_name}_${TIMESTAMP}.gnuplot" \
           "$BASE_URL$endpoint" > "$output_file" 2>&1
    else
        ab -n "$requests" -c "$concurrency" \
           -g "$RESULTS_DIR/${test_name}_${TIMESTAMP}.gnuplot" \
           "$BASE_URL$endpoint" > "$output_file" 2>&1
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Test completed successfully${NC}"
        
        # Extract key metrics
        local rps=$(grep "Requests per second" "$output_file" | awk '{print $4}')
        local mean_time=$(grep "Time per request.*mean" "$output_file" | head -1 | awk '{print $4}')
        local failed=$(grep "Failed requests" "$output_file" | awk '{print $3}')
        
        echo "  - RPS: $rps"
        echo "  - Mean Response Time: ${mean_time}ms"
        echo "  - Failed Requests: $failed"
    else
        echo -e "${RED}✗ Test failed${NC}"
    fi
    echo ""
}

# Helper function to create test data
create_test_user() {
    local login="testuser_$(date +%s%N)"
    local email="${login}@example.com"
    
    cat > /tmp/test_user.json << EOF
{
    "login": "$login",
    "first_name": "Test",
    "last_name": "User",
    "email": "$email",
    "password_hash": "hashed_password_123"
}
EOF
    
    echo "/tmp/test_user.json"
}

# Helper function to create test folder
create_test_folder() {
    local name="TestFolder_$(date +%s%N)"
    
    cat > /tmp/test_folder.json << EOF
{
    "name": "$name",
    "owner_id": "507f1f77bcf86cd799439011"
}
EOF
    
    echo "/tmp/test_folder.json"
}

# Helper function to create test message
create_test_message() {
    local subject="Test Message $(date +%s%N)"
    
    cat > /tmp/test_message.json << EOF
{
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "subject": "$subject",
    "body": "This is a test message for load testing.",
    "is_read": false
}
EOF
    
    echo "/tmp/test_message.json"
}

# Phase 1: Warm-up tests
echo -e "${BLUE}=== Phase 1: Warm-up Tests ===${NC}"
echo ""

run_load_test "warmup_ping" "/ping" "GET" "" 100 10
run_load_test "warmup_swagger" "/swagger/" "GET" "" 50 5

# Phase 2: User Service Load Tests
echo -e "${BLUE}=== Phase 2: User Service Load Tests ===${NC}"
echo ""

# Test 1: Create users (light load)
echo -e "${YELLOW}Test 1: Create Users (Light Load)${NC}"
user_data=$(create_test_user)
run_load_test "user_create_light" "/api/v1/users" "POST" "$user_data" 100 10

# Test 2: Find users by login (read-heavy)
echo -e "${YELLOW}Test 2: Find Users by Login (Read-Heavy)${NC}"
run_load_test "user_find_light" "/api/v1/users/testuser" "GET" "" 500 50

# Test 3: Search users (read-heavy)
echo -e "${YELLOW}Test 3: Search Users (Read-Heavy)${NC}"
run_load_test "user_search_light" "/api/v1/users/search?name_mask=Test" "GET" "" 300 30

# Phase 3: Folder Service Load Tests
echo -e "${BLUE}=== Phase 3: Folder Service Load Tests ===${NC}"
echo ""

# Test 4: Create folders
echo -e "${YELLOW}Test 4: Create Folders${NC}"
folder_data=$(create_test_folder)
run_load_test "folder_create_light" "/api/v1/folders" "POST" "$folder_data" 100 10

# Test 5: List folders (read-heavy)
echo -e "${YELLOW}Test 5: List Folders (Read-Heavy)${NC}"
run_load_test "folder_list_light" "/api/v1/folders?owner_id=507f1f77bcf86cd799439011" "GET" "" 500 50

# Phase 4: Message Service Load Tests
echo -e "${BLUE}=== Phase 4: Message Service Load Tests ===${NC}"
echo ""

# Test 6: Create messages
echo -e "${YELLOW}Test 6: Create Messages${NC}"
message_data=$(create_test_message)
run_load_test "message_create_light" "/api/v1/messages" "POST" "$message_data" 100 10

# Test 7: List messages (read-heavy)
echo -e "${YELLOW}Test 7: List Messages (Read-Heavy)${NC}"
run_load_test "message_list_light" "/api/v1/messages?folder_id=507f1f77bcf86cd799439012" "GET" "" 500 50

# Test 8: Get message by ID
echo -e "${YELLOW}Test 8: Get Message by ID${NC}"
run_load_test "message_get_light" "/api/v1/messages/507f1f77bcf86cd799439013" "GET" "" 300 30

# Phase 5: High Load Tests
echo -e "${BLUE}=== Phase 5: High Load Tests ===${NC}"
echo ""

# Test 9: Mixed workload
echo -e "${YELLOW}Test 9: Mixed Workload (High Load)${NC}"
echo "Running concurrent tests on all services..."

# Run multiple tests in parallel
run_load_test "mixed_user_find" "/api/v1/users/testuser" "GET" "" 1000 100 &
PID1=$!

run_load_test "mixed_folder_list" "/api/v1/folders?owner_id=507f1f77bcf86cd799439011" "GET" "" 1000 100 &
PID2=$!

run_load_test "mixed_message_list" "/api/v1/messages?folder_id=507f1f77bcf86cd799439012" "GET" "" 1000 100 &
PID3=$!

# Wait for all tests to complete
wait $PID1
wait $PID2
wait $PID3

echo -e "${GREEN}✓ Mixed workload test completed${NC}"
echo ""

# Phase 6: Stress Tests
echo -e "${BLUE}=== Phase 6: Stress Tests ===${NC}"
echo ""

# Test 10: Extreme load
echo -e "${YELLOW}Test 10: Extreme Load (Stress Test)${NC}"
run_load_test "stress_ping" "/ping" "GET" "" 10000 200

# Test 11: Sustained load
echo -e "${YELLOW}Test 11: Sustained Load (Duration Test)${NC}"
echo "Running sustained load for 60 seconds..."

for i in {1..6}; do
    echo "  - Iteration $i/6"
    run_load_test "sustained_user_find_$i" "/api/v1/users/testuser" "GET" "" 1000 50
    sleep 10
done

# Phase 7: Generate Summary Report
echo -e "${BLUE}=== Phase 7: Summary Report ===${NC}"
echo ""

echo "Load Testing Summary"
echo "===================="
echo "Timestamp: $TIMESTAMP"
echo "Results Directory: $RESULTS_DIR"
echo ""

# Count total tests
total_tests=$(ls -1 "$RESULTS_DIR"/*.txt 2>/dev/null | wc -l)
echo "Total tests executed: $total_tests"
echo ""

# Find best and worst performing tests
echo "Performance Summary:"
echo "-------------------"

# Find highest RPS
highest_rps=0
best_test=""
for file in "$RESULTS_DIR"/*.txt; do
    if [ -f "$file" ]; then
        rps=$(grep "Requests per second" "$file" | awk '{print $4}' | cut -d'.' -f1)
        if [ ! -z "$rps" ] && [ "$rps" -gt "$highest_rps" ]; then
            highest_rps=$rps
            best_test=$(basename "$file" .txt)
        fi
    fi
done
echo "Best performing test: $best_test ($highest_rps RPS)"

# Find lowest RPS
lowest_rps=999999
worst_test=""
for file in "$RESULTS_DIR"/*.txt; do
    if [ -f "$file" ]; then
        rps=$(grep "Requests per second" "$file" | awk '{print $4}' | cut -d'.' -f1)
        if [ ! -z "$rps" ] && [ "$rps" -lt "$lowest_rps" ]; then
            lowest_rps=$rps
            worst_test=$(basename "$file" .txt)
        fi
    fi
done
echo "Worst performing test: $worst_test ($lowest_rps RPS)"

echo ""
echo -e "${GREEN}=== Load Testing Complete ===${NC}"
echo "Check Grafana dashboards for detailed metrics:"
echo "  - http://localhost:3000 (admin/admin)"
echo "  - Email Service Overview dashboard"
echo "  - MongoDB Performance dashboard"
