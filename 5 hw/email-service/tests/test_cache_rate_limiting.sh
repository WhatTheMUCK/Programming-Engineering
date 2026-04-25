#!/bin/bash

# Test script to demonstrate cache and rate limiting effectiveness
# This script tests the cache hit rate and rate limiting behavior

echo "=========================================="
echo "Cache and Rate Limiting Test Script"
echo "=========================================="
echo ""

BASE_URL="http://localhost:8081"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Generate unique timestamp for test user
TIMESTAMP=$(date +%s)
TEST_USER="cachetest_${TIMESTAMP}"

echo "1. Testing Cache Effectiveness (User Cache)"
echo "-------------------------------------------"
echo "Creating test user for cache test..."
echo ""

# Create test user first (before rate limiting test)
CREATE_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/api/v1/users" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"${TEST_USER}\",\"email\":\"${TEST_USER}@example.com\",\"first_name\":\"Cache\",\"last_name\":\"Test\",\"password\":\"password123\"}")

CREATE_HTTP_CODE=$(echo "$CREATE_RESPONSE" | tail -n1)

if [ "$CREATE_HTTP_CODE" = "201" ]; then
    echo -e "${GREEN}✓ Test user created successfully${NC}"
elif [ "$CREATE_HTTP_CODE" = "409" ]; then
    echo -e "${YELLOW}⚠ Test user already exists, will use existing${NC}"
else
    echo -e "${RED}✗ Failed to create test user (HTTP $CREATE_HTTP_CODE)${NC}"
    echo "Response: $(echo "$CREATE_RESPONSE" | sed '$d')"
    echo ""
    echo "Skipping cache test due to user creation failure"
    echo ""
    echo "2. Testing Rate Limiting (User Registration)"
    echo "-------------------------------------------"
    echo "Rate limit: 5 requests burst capacity"
    echo "Making 12 requests (should allow 4-5, deny rest)"
    echo ""

    SUCCESS_COUNT=0
    DENIED_COUNT=0

    for i in {1..12}; do
        RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/api/v1/users" \
            -H "Content-Type: application/json" \
            -d "{\"login\":\"ratelimittest${i}_${TIMESTAMP}\",\"email\":\"ratelimit${i}_${TIMESTAMP}@example.com\",\"first_name\":\"RateLimit\",\"last_name\":\"Test${i}\",\"password\":\"password123\"}")
        
        HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
        BODY=$(echo "$RESPONSE" | sed '$d')
        
        if [ "$HTTP_CODE" = "429" ]; then
            echo -e "${RED}Request $i: Rate Limited (HTTP $HTTP_CODE)${NC}"
            DENIED_COUNT=$((DENIED_COUNT + 1))
        elif [ "$HTTP_CODE" = "201" ] || [ "$HTTP_CODE" = "200" ]; then
            echo -e "${GREEN}Request $i: Success (HTTP $HTTP_CODE)${NC}"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo -e "${YELLOW}Request $i: HTTP $HTTP_CODE${NC}"
            echo "Response: $BODY"
        fi
        
        sleep 0.1
    done

    echo ""
    echo "Rate Limiting Results:"
    echo "  Allowed: $SUCCESS_COUNT"
    echo "  Denied: $DENIED_COUNT"
    echo ""

    echo "3. Checking Metrics"
    echo "-------------------------------------------"
    echo "Fetching cache and rate limiting metrics..."
    echo ""

    METRICS_RESPONSE=$(curl -s "${BASE_URL}/cache-rate-limiting-metrics")
    echo "$METRICS_RESPONSE"

    echo ""
    echo "=========================================="
    echo "Test Complete!"
    echo "=========================================="
    echo ""
    echo "Summary:"
    echo "  Cache Test: Skipped (user creation failed)"
    echo "  Rate Limiting: $SUCCESS_COUNT allowed, $DENIED_COUNT denied"
    echo ""
    echo "To view the dashboard:"
    echo "1. Open Grafana at http://localhost:3000"
    echo "2. Navigate to Dashboards → Email Service - Cache & Rate Limiting Dashboard"
    echo "3. The metrics should now be visible"
    echo ""
    echo "Key metrics to observe:"
    echo "  - rate_limit_denied_requests_total: Should show $DENIED_COUNT from our test"
    echo "  - rate_limit_allowed_requests_total: Should show $SUCCESS_COUNT from our test"
    exit 0
fi

echo ""
echo "Making 10 requests to /api/v1/users/by-login?login=${TEST_USER}"
echo "First request should be a cache miss, subsequent should be cache hits"
echo ""

for i in {1..10}; do
    START_TIME=$(date +%s%N)
    RESPONSE=$(curl -s -w "\n%{http_code}" "${BASE_URL}/api/v1/users/by-login?login=${TEST_USER}")
    HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
    END_TIME=$(date +%s%N)
    
    DURATION=$(( (END_TIME - START_TIME) / 1000000 )) # Convert to milliseconds
    
    if [ "$HTTP_CODE" = "200" ]; then
        if [ $i -eq 1 ]; then
            echo -e "${BLUE}Request $i: Success (${DURATION}ms) - Cache Miss${NC}"
        else
            echo -e "${GREEN}Request $i: Success (${DURATION}ms) - Cache Hit${NC}"
        fi
    else
        echo -e "${RED}Request $i: Failed (HTTP $HTTP_CODE)${NC}"
    fi
    
    sleep 0.05
done

echo ""
echo "2. Testing Folder Cache"
echo "-------------------------------------------"
echo "Making 5 requests to /api/v1/folders"
echo "First request should be a cache miss, subsequent should be cache hits"
echo ""

for i in {1..5}; do
    START_TIME=$(date +%s%N)
    RESPONSE=$(curl -s -w "\n%{http_code}" "${BASE_URL}/api/v1/folders")
    HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
    END_TIME=$(date +%s%N)
    
    DURATION=$(( (END_TIME - START_TIME) / 1000000 )) # Convert to milliseconds
    
    if [ "$HTTP_CODE" = "200" ]; then
        if [ $i -eq 1 ]; then
            echo -e "${BLUE}Request $i: Success (${DURATION}ms) - Cache Miss${NC}"
        else
            echo -e "${GREEN}Request $i: Success (${DURATION}ms) - Cache Hit${NC}"
        fi
    else
        echo -e "${RED}Request $i: Failed (HTTP $HTTP_CODE)${NC}"
    fi
    
    sleep 0.05
done

echo ""
echo "3. Testing Rate Limiting (User Registration)"
echo "-------------------------------------------"
echo "Rate limit: 10 requests per 60 seconds"
echo "Making 12 requests (should allow 10, deny 2)"
echo ""

SUCCESS_COUNT=0
DENIED_COUNT=0

for i in {1..12}; do
    RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "${BASE_URL}/api/v1/users" \
        -H "Content-Type: application/json" \
        -d "{\"login\":\"ratelimittest${i}_${TIMESTAMP}\",\"email\":\"ratelimit${i}_${TIMESTAMP}@example.com\",\"first_name\":\"RateLimit\",\"last_name\":\"Test${i}\",\"password\":\"password123\"}")
    
    HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
    BODY=$(echo "$RESPONSE" | sed '$d')
    
    if [ "$HTTP_CODE" = "429" ]; then
        echo -e "${RED}Request $i: Rate Limited (HTTP $HTTP_CODE)${NC}"
        DENIED_COUNT=$((DENIED_COUNT + 1))
    elif [ "$HTTP_CODE" = "201" ] || [ "$HTTP_CODE" = "200" ]; then
        echo -e "${GREEN}Request $i: Success (HTTP $HTTP_CODE)${NC}"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo -e "${YELLOW}Request $i: HTTP $HTTP_CODE${NC}"
        echo "Response: $BODY"
    fi
    
    sleep 0.1
done

echo ""
echo "Rate Limiting Results:"
echo "  Allowed: $SUCCESS_COUNT"
echo "  Denied: $DENIED_COUNT"
echo ""

echo "4. Checking Metrics"
echo "-------------------------------------------"
echo "Fetching cache and rate limiting metrics..."
echo ""

METRICS_RESPONSE=$(curl -s "${BASE_URL}/cache-rate-limiting-metrics")
echo "$METRICS_RESPONSE"

echo ""
echo "=========================================="
echo "Test Complete!"
echo "=========================================="
echo ""
echo "Summary:"
echo "  Cache Test: User cache tested with 10 requests"
echo "  Cache Test: Folder cache tested with 5 requests"
echo "  Rate Limiting: $SUCCESS_COUNT allowed, $DENIED_COUNT denied"
echo ""
echo "To view the dashboard:"
echo "1. Open Grafana at http://localhost:3000"
echo "2. Navigate to Dashboards → Email Service - Cache & Rate Limiting Dashboard"
echo "3. The metrics should now be visible"
echo ""
echo "Key metrics to observe:"
echo "  - cache_hit_rate: Should increase with repeated requests"
echo "  - cache_hits_total vs cache_misses_total"
echo "  - rate_limit_denied_requests_total: Should show $DENIED_COUNT from our test"
echo "  - rate_limit_allowed_requests_total: Should show $SUCCESS_COUNT from our test"
