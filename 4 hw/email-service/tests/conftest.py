import pytest
import aiohttp
import os
import json
import logging
import sys
from pymongo import MongoClient
from pymongo.errors import ConnectionFailure, OperationFailure

# Get service URL from environment or use default test gateway
SERVICE_URL = os.getenv('SERVICE_URL', 'http://host.containers.internal:18080')

MONGO_URI = os.getenv('MONGO_URI', 'mongodb://mongo-test:27017/')
DB_NAME = os.getenv('DB_NAME', 'email_test_db')

logger = logging.getLogger(__name__)


class Response:
    """Response wrapper for HTTP responses"""
    
    def __init__(self, status, text, headers):
        self.status = status
        self.text = text
        self.headers = headers
        print(f"[DEBUG] Response created: status={status}, text_len={len(text)}, text={repr(text[:50])}", file=sys.stderr)
    
    def json(self):
        """Parse response as JSON"""
        if not self.text:
            raise ValueError(f"Empty response body. Status: {self.status}")
        return json.loads(self.text)


class ServiceClient:
    """Simple HTTP client for testing services"""
    
    def __init__(self, base_url, session):
        self.base_url = base_url
        self.session = session
        print(f"[DEBUG] ServiceClient initialized with base_url={base_url}", file=sys.stderr)
    
    async def get(self, path, **kwargs):
        """Make GET request"""
        url = f"{self.base_url}{path}"
        print(f"[DEBUG] GET {url}", file=sys.stderr)
        try:
            async with self.session.get(url, **kwargs) as resp:
                text = await resp.text()
                print(f"[DEBUG] Response status: {resp.status}, text_len: {len(text)}, text: {repr(text[:100])}", file=sys.stderr)
                return Response(resp.status, text, resp.headers)
        except Exception as e:
            print(f"[ERROR] GET request failed: {e}", file=sys.stderr)
            raise
    
    async def post(self, path, json=None, **kwargs):
        """Make POST request"""
        url = f"{self.base_url}{path}"
        print(f"[DEBUG] POST {url}", file=sys.stderr)
        try:
            async with self.session.post(url, json=json, **kwargs) as resp:
                text = await resp.text()
                print(f"[DEBUG] Response status: {resp.status}, text_len: {len(text)}", file=sys.stderr)
                return Response(resp.status, text, resp.headers)
        except Exception as e:
            print(f"[ERROR] POST request failed: {e}", file=sys.stderr)
            raise
    
    async def put(self, path, json=None, **kwargs):
        """Make PUT request"""
        url = f"{self.base_url}{path}"
        print(f"[DEBUG] PUT {url}", file=sys.stderr)
        try:
            async with self.session.put(url, json=json, **kwargs) as resp:
                text = await resp.text()
                print(f"[DEBUG] Response status: {resp.status}, text_len: {len(text)}", file=sys.stderr)
                return Response(resp.status, text, resp.headers)
        except Exception as e:
            print(f"[ERROR] PUT request failed: {e}", file=sys.stderr)
            raise
    
    async def delete(self, path, **kwargs):
        """Make DELETE request"""
        url = f"{self.base_url}{path}"
        print(f"[DEBUG] DELETE {url}", file=sys.stderr)
        try:
            async with self.session.delete(url, **kwargs) as resp:
                text = await resp.text()
                print(f"[DEBUG] Response status: {resp.status}, text_len: {len(text)}", file=sys.stderr)
                return Response(resp.status, text, resp.headers)
        except Exception as e:
            print(f"[ERROR] DELETE request failed: {e}", file=sys.stderr)
            raise


def clear_database():
    """Clear all test data from the test database"""
    try:
        print(f"[DEBUG] Attempting to connect to test database: {MONGO_URI}{DB_NAME}", file=sys.stderr)
        client = MongoClient(MONGO_URI, serverSelectionTimeoutMS=5000)
        db = client[DB_NAME]
        
        # Test connection
        client.admin.command('ping')
        print(f"[DEBUG] Successfully connected to test database", file=sys.stderr)
        
        # Delete all data from collections
        result_messages = db.messages.delete_many({})
        result_folders = db.folders.delete_many({})
        result_users = db.users.delete_many({})
        
        print(f"[DEBUG] Test database cleared successfully.", file=sys.stderr)
        print(f"[DEBUG] Deleted {result_messages.deleted_count} messages", file=sys.stderr)
        print(f"[DEBUG] Deleted {result_folders.deleted_count} folders", file=sys.stderr)
        print(f"[DEBUG] Deleted {result_users.deleted_count} users", file=sys.stderr)
        
        # Verify deletion
        user_count = db.users.count_documents({})
        print(f"[DEBUG] Users remaining: {user_count}", file=sys.stderr)
        
        client.close()
    except ConnectionFailure as e:
        print(f"[WARNING] Failed to connect to test database: {e}", file=sys.stderr)
        print(f"[WARNING] MONGO_URI={MONGO_URI}, DB_NAME={DB_NAME}", file=sys.stderr)
        print(f"[WARNING] Make sure mongo-test container is running with --profile test", file=sys.stderr)
    except OperationFailure as e:
        print(f"[WARNING] Failed to clear test database: {e}", file=sys.stderr)
    except Exception as e:
        print(f"[WARNING] Failed to clear test database: {e}", file=sys.stderr)


@pytest.fixture(scope="function", autouse=True)
def clear_db_before_test():
    """Clear test database before each test"""
    clear_database()
    yield


@pytest.fixture
async def aiohttp_session():
    """Create aiohttp session"""
    print("[DEBUG] Creating aiohttp session", file=sys.stderr)
    async with aiohttp.ClientSession() as session:
        yield session


@pytest.fixture
async def service_client(aiohttp_session):
    """Fixture for service client (via nginx gateway)"""
    print(f"[DEBUG] Creating service_client with SERVICE_URL={SERVICE_URL}", file=sys.stderr)
    return ServiceClient(SERVICE_URL, aiohttp_session)
