import pytest
import aiohttp
import os
import json
import logging
import sys
import psycopg2
from psycopg2 import sql

# Get service URL from environment or use default (nginx gateway)
SERVICE_URL = os.getenv('SERVICE_URL', 'http://localhost:8080')

DB_HOST = os.getenv('DB_HOST', 'postgres-test')
DB_PORT = os.getenv('DB_PORT', '5432')
DB_NAME = os.getenv('DB_NAME', 'email_test_db')
DB_USER = os.getenv('DB_USER', 'email_user')
DB_PASSWORD = os.getenv('DB_PASSWORD', 'email_pass')

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
        print(f"[DEBUG] Attempting to connect to test database: {DB_HOST}:{DB_PORT}/{DB_NAME}", file=sys.stderr)
        conn = psycopg2.connect(
            host=DB_HOST,
            port=int(DB_PORT),
            database=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD,
            connect_timeout=5
        )
        cursor = conn.cursor()
        
        print(f"[DEBUG] Successfully connected to test database", file=sys.stderr)
        
        # Disable foreign key constraints temporarily
        cursor.execute("SET session_replication_role = 'replica';")
        
        # Delete all data from tables
        cursor.execute("DELETE FROM messages;")
        cursor.execute("DELETE FROM folders;")
        cursor.execute("DELETE FROM users;")
        
        # Reset sequences
        cursor.execute("ALTER SEQUENCE users_id_seq RESTART WITH 1;")
        cursor.execute("ALTER SEQUENCE folders_id_seq RESTART WITH 1;")
        cursor.execute("ALTER SEQUENCE messages_id_seq RESTART WITH 1;")
        
        # Re-enable foreign key constraints
        cursor.execute("SET session_replication_role = 'origin';")
        
        conn.commit()
        
        # Verify deletion
        cursor.execute("SELECT COUNT(*) FROM users;")
        user_count = cursor.fetchone()[0]
        print(f"[DEBUG] Test database cleared successfully. Users remaining: {user_count}", file=sys.stderr)
        
        cursor.close()
        conn.close()
    except psycopg2.OperationalError as e:
        print(f"[WARNING] Failed to connect to test database: {e}", file=sys.stderr)
        print(f"[WARNING] DB_HOST={DB_HOST}, DB_PORT={DB_PORT}, DB_NAME={DB_NAME}, DB_USER={DB_USER}", file=sys.stderr)
        print(f"[WARNING] Make sure postgres-test container is running with --profile test", file=sys.stderr)
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
