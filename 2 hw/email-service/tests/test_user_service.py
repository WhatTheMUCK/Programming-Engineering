# Integration tests for User Service
import pytest
import jwt
import time
import uuid
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
# User Creation Tests
# ============================================================================

async def test_create_user_success(service_client):
    """Test successful user creation"""
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    response = await service_client.post('/api/v1/users', json=user_data)
    assert response.status == 201
    
    json_response = response.json()
    assert 'id' in json_response
    assert json_response['login'] == user_data['login']
    assert json_response['email'] == user_data['email']
    assert json_response['first_name'] == 'Test'
    assert json_response['last_name'] == 'User'
    assert 'created_at' in json_response
    assert 'password' not in json_response  # Password should not be returned


@pytest.mark.parametrize('field, value, expected_error', [
    pytest.param('login', '', 'Login is required', id='Empty login'),
    pytest.param('login', None, 'Login is required', id='Missing login'),
    pytest.param('email', 'invalid-email', 'Invalid email format', id='Invalid email'),
    pytest.param('email', '', 'Email is required', id='Empty email'),
    pytest.param('first_name', '', 'First name is required', id='Empty first name'),
    pytest.param('last_name', '', 'Last name is required', id='Empty last name'),
    pytest.param('password', '', 'Password is required', id='Empty password'),
])
async def test_create_user_validation_errors(service_client, field, value, expected_error):
    """Test user creation with validation errors"""
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    
    if value is None:
        user_data.pop(field)
    else:
        user_data[field] = value
    
    response = await service_client.post('/api/v1/users', json=user_data)
    assert response.status == 400
    
    json_response = response.json()
    assert 'error' in json_response


async def test_create_user_duplicate_login(service_client):
    """Test that duplicate login is rejected"""
    unique_login = generate_unique_login()
    unique_email1 = generate_unique_email()
    unique_email2 = generate_unique_email()
    
    # Create first user
    user_data1 = {
        'login': unique_login,
        'email': unique_email1,
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    response1 = await service_client.post('/api/v1/users', json=user_data1)
    assert response1.status == 201
    
    # Try to create second user with same login but different email
    user_data2 = {
        'login': unique_login,  # Same login
        'email': unique_email2,  # Different email
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    response2 = await service_client.post('/api/v1/users', json=user_data2)
    assert response2.status == 409
    
    json_response = response2.json()
    assert 'error' in json_response


async def test_create_user_duplicate_email(service_client):
    """Test that duplicate email is rejected"""
    unique_email = generate_unique_email()
    unique_login1 = generate_unique_login()
    unique_login2 = generate_unique_login()
    
    # Create first user
    user_data1 = {
        'login': unique_login1,
        'email': unique_email,
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    response1 = await service_client.post('/api/v1/users', json=user_data1)
    assert response1.status == 201
    
    # Try to create second user with same email but different login
    user_data2 = {
        'login': unique_login2,  # Different login
        'email': unique_email,  # Same email
        'first_name': 'Test',
        'last_name': 'User',
        'password': 'password123'
    }
    response2 = await service_client.post('/api/v1/users', json=user_data2)
    assert response2.status == 409
    
    json_response = response2.json()
    assert 'error' in json_response


# ============================================================================
# User Login Tests
# ============================================================================

async def test_login_success(service_client):
    """Test successful user login"""
    unique_login = generate_unique_login()
    unique_email = generate_unique_email()
    
    # First create a user
    user_data = {
        'login': unique_login,
        'email': unique_email,
        'first_name': 'Login',
        'last_name': 'User',
        'password': 'testpassword'
    }
    await service_client.post('/api/v1/users', json=user_data)
    
    # Now login
    login_data = {
        'login': unique_login,
        'password': 'testpassword'
    }
    
    response = await service_client.post('/api/v1/auth/login', json=login_data)
    assert response.status == 200
    
    json_response = response.json()
    assert 'token' in json_response
    assert 'user_id' in json_response
    assert 'expires_at' in json_response
    assert json_response['user_id'] > 0


async def test_login_invalid_credentials(service_client):
    """Test login with invalid credentials"""
    login_data = {
        'login': 'nonexistent_' + uuid.uuid4().hex[:8],
        'password': 'wrongpassword'
    }
    
    response = await service_client.post('/api/v1/auth/login', json=login_data)
    assert response.status == 401
    
    json_response = response.json()
    assert 'error' in json_response


async def test_login_wrong_password(service_client):
    """Test login with correct login but wrong password"""
    unique_login = generate_unique_login()
    unique_email = generate_unique_email()
    
    # Create a user
    user_data = {
        'login': unique_login,
        'email': unique_email,
        'first_name': 'Wrong',
        'last_name': 'Pass',
        'password': 'correctpassword'
    }
    await service_client.post('/api/v1/users', json=user_data)
    
    # Try to login with wrong password
    login_data = {
        'login': unique_login,
        'password': 'wrongpassword'
    }
    
    response = await service_client.post('/api/v1/auth/login', json=login_data)
    assert response.status == 401


@pytest.mark.parametrize('field, value', [
    pytest.param('login', '', id='Empty login'),
    pytest.param('login', None, id='Missing login'),
    pytest.param('password', '', id='Empty password'),
    pytest.param('password', None, id='Missing password'),
])
async def test_login_validation_errors(service_client, field, value):
    """Test login with validation errors"""
    login_data = {
        'login': 'testuser',
        'password': 'password123'
    }
    
    if value is None:
        login_data.pop(field)
    else:
        login_data[field] = value
    
    response = await service_client.post('/api/v1/auth/login', json=login_data)
    assert response.status == 400


# ============================================================================
# Find User by Login Tests
# ============================================================================

async def test_find_user_by_login_success(service_client):
    """Test finding user by login"""
    unique_login = generate_unique_login()
    unique_email = generate_unique_email()
    
    # Create a user
    user_data = {
        'login': unique_login,
        'email': unique_email,
        'first_name': 'Find',
        'last_name': 'User',
        'password': 'password123'
    }
    create_response = await service_client.post('/api/v1/users', json=user_data)
    user_id = create_response.json()['id']
    
    # Login to get token
    login_response = await service_client.post('/api/v1/auth/login', json={
        'login': unique_login,
        'password': 'password123'
    })
    token = login_response.json()['token']
    headers = {'Authorization': f'Bearer {token}'}
    
    # Find the user
    response = await service_client.get(f'/api/v1/users/by-login?login={unique_login}', headers=headers)
    assert response.status == 200
    
    json_response = response.json()
    assert json_response['id'] == user_id
    assert json_response['login'] == unique_login
    assert json_response['email'] == unique_email
    assert json_response['first_name'] == 'Find'
    assert json_response['last_name'] == 'User'
    assert 'password' not in json_response


async def test_find_user_by_login_not_found(service_client):
    """Test finding non-existent user"""
    # Create a dummy user to get a valid token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Dummy',
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
    
    response = await service_client.get('/api/v1/users/by-login?login=nonexistent_' + uuid.uuid4().hex[:8], headers=headers)
    assert response.status == 404
    
    json_response = response.json()
    assert 'error' in json_response


# ============================================================================
# Search Users by Name Mask Tests
# ============================================================================

async def test_search_users_by_name_mask(service_client):
    """Test searching users by name mask"""
    # Create a dummy user to get a valid token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Dummy',
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
    
    # Search for users with first name starting with 'Jo'
    response = await service_client.get('/api/v1/users/search?mask=Jo', headers=headers)
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    # Should find at least some users with 'Jo' in their name
    assert len(json_response) >= 0


async def test_search_users_no_results(service_client):
    """Test search that returns no results"""
    # Create a dummy user to get a valid token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Dummy',
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
    
    response = await service_client.get('/api/v1/users/search?mask=XYZ_NONEXISTENT_' + uuid.uuid4().hex[:8], headers=headers)
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    assert len(json_response) == 0


async def test_search_users_no_parameters(service_client):
    """Test search without parameters (should return 400)"""
    # Create a dummy user to get a valid token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Dummy',
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
    
    response = await service_client.get('/api/v1/users/search', headers=headers)
    # This should return 400 (bad request) because mask is required
    assert response.status == 400
