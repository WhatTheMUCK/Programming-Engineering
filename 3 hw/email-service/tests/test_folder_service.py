# Integration tests for Folder Service
import pytest
import jwt
import uuid
import time
from testsuite.utils import matching

JWT_SECRET_KEY = 'your-secret-key-change-in-production'

# Helper functions
def make_jwt_token(user_id):
    """Create a JWT token for testing"""
    payload = {'user_id': user_id}
    return jwt.encode(payload, JWT_SECRET_KEY, algorithm='HS256')

def make_auth_headers(user_id):
    """Create authorization headers with JWT token"""
    token = make_jwt_token(user_id)
    return {'Authorization': f'Bearer {token}'}

def generate_unique_login():
    """Generate a unique login for testing"""
    return f"testuser_{int(time.time() * 1000)}_{uuid.uuid4().hex[:8]}"

def generate_unique_email():
    """Generate a unique email for testing"""
    return f"test_{int(time.time() * 1000)}_{uuid.uuid4().hex[:8]}@example.com"


# ============================================================================
# Health Check Tests
# ============================================================================

async def test_ping(service_client):
    """Test that the service responds to ping"""
    response = await service_client.get('/ping')
    assert response.status == 200
    assert response.text == 'pong\n'


# ============================================================================
# Create Folder Tests
# ============================================================================

async def test_create_folder_success(service_client):
    """Test successful folder creation"""
    # First create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Folder',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    user_id = create_response.json()['id']
    
    # Login to get token
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_data = {
        'name': 'Inbox'
    }
    
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    assert response.status == 201
    
    json_response = response.json()
    assert 'id' in json_response
    assert json_response['name'] == 'Inbox'
    assert json_response['user_id'] == user_id
    assert 'created_at' in json_response


async def test_create_folder_without_auth(service_client):
    """Test that folder creation requires authentication"""
    folder_data = {
        'name': 'Inbox'
    }
    
    response = await service_client.post('/api/v1/folders', json=folder_data)
    assert response.status == 400


async def test_create_folder_invalid_token(service_client):
    """Test folder creation with invalid token"""
    headers = {'Authorization': 'Bearer invalid_token'}
    folder_data = {
        'name': 'Inbox'
    }
    
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    assert response.status == 403


@pytest.mark.parametrize('field, value, expected_error', [
    pytest.param('name', '', 'Name is required', id='Empty name'),
    pytest.param('name', None, 'Name is required', id='Missing name'),
])
async def test_create_folder_validation_errors(service_client, field, value, expected_error):
    """Test folder creation with validation errors"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_data = {'name': 'TestFolder'}
    
    if value is None:
        folder_data.pop(field)
    else:
        folder_data[field] = value
    
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    assert response.status == 400


async def test_create_multiple_folders_for_user(service_client):
    """Test creating multiple folders for the same user"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_names = ['Inbox', 'Sent', 'Drafts', 'Trash']
    folder_ids = []
    
    for name in folder_names:
        folder_data = {'name': name}
        response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
        assert response.status == 201
        folder_ids.append(response.json()['id'])
    
    # Verify all folders were created with unique IDs
    assert len(set(folder_ids)) == len(folder_ids)


# ============================================================================
# List Folders Tests
# ============================================================================

async def test_list_folders_success(service_client):
    """Test listing all folders for a user"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    # Create some folders
    folder_names = ['Inbox', 'Sent', 'Drafts']
    for name in folder_names:
        folder_data = {'name': name}
        await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    
    # List folders
    response = await service_client.get('/api/v1/folders', headers=headers)
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    assert len(json_response) >= len(folder_names)
    
    # Verify folder names
    returned_names = [folder['name'] for folder in json_response]
    for name in folder_names:
        assert name in returned_names


async def test_list_folders_without_auth(service_client):
    """Test that listing folders requires authentication"""
    response = await service_client.get('/api/v1/folders')
    assert response.status == 400


async def test_list_folders_empty(service_client):
    """Test listing folders when user has no folders"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    response = await service_client.get('/api/v1/folders', headers=headers)
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    assert len(json_response) == 0


async def test_list_folders_user_isolation(service_client):
    """Test that users can only see their own folders"""
    # Create user 1 and get token
    user1_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'User1',
        'last_name': 'Test',
        'password': 'password123'
    }
    
    create1_response = await service_client.post('/api/v1/users', json=user1_data)
    
    login1_response = await service_client.post('/api/v1/auth/login', json={
        'login': user1_data['login'],
        'password': 'password123'
    })
    token1 = login1_response.json()['token']
    headers1 = {'Authorization': f'Bearer {token1}'}
    
    # Create user 2 and get token
    user2_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'User2',
        'last_name': 'Test',
        'password': 'password123'
    }
    
    create2_response = await service_client.post('/api/v1/users', json=user2_data)
    
    login2_response = await service_client.post('/api/v1/auth/login', json={
        'login': user2_data['login'],
        'password': 'password123'
    })
    token2 = login2_response.json()['token']
    headers2 = {'Authorization': f'Bearer {token2}'}
    
    # Create folders for user 1
    folder_data = {'name': 'User1Folder'}
    await service_client.post('/api/v1/folders', json=folder_data, headers=headers1)
    
    # Create folders for user 2
    folder_data = {'name': 'User2Folder'}
    await service_client.post('/api/v1/folders', json=folder_data, headers=headers2)
    
    # List folders for user 1
    response1 = await service_client.get('/api/v1/folders', headers=headers1)
    folders1 = response1.json()
    folder_names1 = [f['name'] for f in folders1]
    
    # List folders for user 2
    response2 = await service_client.get('/api/v1/folders', headers=headers2)
    folders2 = response2.json()
    folder_names2 = [f['name'] for f in folders2]
    
    # Verify isolation
    assert 'User1Folder' in folder_names1
    assert 'User1Folder' not in folder_names2
    assert 'User2Folder' in folder_names2
    assert 'User2Folder' not in folder_names1


# ============================================================================
# Folder Structure Tests
# ============================================================================

async def test_folder_response_structure(service_client):
    """Test that folder response has correct structure"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_data = {'name': 'TestFolder'}
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    
    json_response = response.json()
    
    # Verify all expected fields are present
    assert 'id' in json_response
    assert 'name' in json_response
    assert 'user_id' in json_response
    assert 'created_at' in json_response
    
    # Verify types
    assert isinstance(json_response['id'], int)
    assert isinstance(json_response['name'], str)
    assert isinstance(json_response['user_id'], int)
    assert isinstance(json_response['created_at'], str)


async def test_list_folders_response_structure(service_client):
    """Test that list folders response has correct structure"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    response = await service_client.get('/api/v1/folders', headers=headers)
    
    json_response = response.json()
    
    # Verify response structure
    assert isinstance(json_response, list)
    
    # If there are folders, verify their structure
    if json_response:
        folder = json_response[0]
        assert 'id' in folder
        assert 'name' in folder
        assert 'user_id' in folder
        assert 'created_at' in folder


# ============================================================================
# Edge Cases and Error Handling
# ============================================================================

async def test_create_folder_with_special_characters(service_client):
    """Test creating folder with special characters in name"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_data = {'name': 'My Folder (2024)'}
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    
    # Should either succeed or fail gracefully
    assert response.status in [201, 400]


async def test_create_folder_with_long_name(service_client):
    """Test creating folder with very long name (exceeds VARCHAR(255) limit)"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    # 1000 characters exceeds the VARCHAR(255) constraint on folders.name
    long_name = 'A' * 1000
    folder_data = {'name': long_name}
    response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    
    # Should fail with 409 Conflict (constraint violation) or 400 Bad Request
    assert response.status in [400, 409]


async def test_create_duplicate_folder_name(service_client):
    """Test creating folders with duplicate names for same user"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    create_response = await service_client.post('/api/v1/users', json=user_data)
    
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': user_data['login'],
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    folder_data = {'name': 'DuplicateFolder'}
    
    # Create first folder
    response1 = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    assert response1.status == 201
    
    # Try to create second folder with same name
    response2 = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    
    # Behavior depends on implementation - might allow or reject
    assert response2.status in [201, 409]
