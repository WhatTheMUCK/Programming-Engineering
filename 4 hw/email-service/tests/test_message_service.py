# Integration tests for Message Service
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
# Create Message Tests
# ============================================================================

async def test_create_message_success(service_client):
    """Test successful message creation"""
    # Create a user and get token
    user_data = {
        'login': generate_unique_login(),
        'email': generate_unique_email(),
        'first_name': 'Message',
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_data = {
        'subject': 'Test Subject',
        'body': 'This is a test message body',
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    assert response.status == 201
    
    json_response = response.json()
    assert 'id' in json_response
    assert json_response['subject'] == 'Test Subject'
    assert json_response['body'] == 'This is a test message body'
    assert json_response['recipient_email'] == 'recipient@example.com'
    assert json_response['folder_id'] == folder_id
    assert 'created_at' in json_response


async def test_create_message_without_auth(service_client):
    """Test that message creation requires authentication"""
    folder_id = 1
    message_data = {
        'subject': 'Test Subject',
        'body': 'Test body',
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data
    )
    assert response.status == 400


async def test_create_message_invalid_token(service_client):
    """Test message creation with invalid token"""
    headers = {'Authorization': 'Bearer invalid_token'}
    folder_id = 1
    message_data = {
        'subject': 'Test Subject',
        'body': 'Test body',
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    assert response.status == 403


@pytest.mark.parametrize('field, value, expected_error', [
    pytest.param('subject', '', 'Subject is required', id='Empty subject'),
    pytest.param('subject', None, 'Subject is required', id='Missing subject'),
    pytest.param('body', '', 'Body is required', id='Empty body'),
    pytest.param('body', None, 'Body is required', id='Missing body'),
    pytest.param('recipient_email', '', 'Recipient is required', id='Empty recipient'),
    pytest.param('recipient_email', None, 'Recipient is required', id='Missing recipient'),
])
async def test_create_message_validation_errors(service_client, field, value, expected_error):
    """Test message creation with validation errors"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_data = {
        'subject': 'Test Subject',
        'body': 'Test body',
        'recipient_email': 'recipient@example.com'
    }
    
    if value is None:
        message_data.pop(field)
    else:
        message_data[field] = value
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    assert response.status == 400


async def test_create_message_invalid_folder(service_client):
    """Test creating message in non-existent folder"""
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
    
    folder_id = 99999  # Non-existent folder
    
    message_data = {
        'subject': 'Test Subject',
        'body': 'Test body',
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    assert response.status in [404, 403]


async def test_create_message_invalid_email_format(service_client):
    """Test creating message with invalid email format"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_data = {
        'subject': 'Test Subject',
        'body': 'Test body',
        'recipient_email': 'invalid-email'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    assert response.status == 400


# ============================================================================
# List Messages Tests
# ============================================================================

async def test_list_messages_success(service_client):
    """Test listing all messages in a folder"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    # Create some messages
    messages = [
        {'subject': 'Message 1', 'body': 'Body 1', 'recipient_email': 'recipient@example.com'},
        {'subject': 'Message 2', 'body': 'Body 2', 'recipient_email': 'recipient@example.com'},
        {'subject': 'Message 3', 'body': 'Body 3', 'recipient_email': 'recipient@example.com'},
    ]
    
    for msg in messages:
        await service_client.post(
            f'/api/v1/folders/{folder_id}/messages',
            json=msg,
            headers=headers
        )
    
    # List messages
    response = await service_client.get(
        f'/api/v1/folders/{folder_id}/messages',
        headers=headers
    )
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    assert len(json_response) >= len(messages)


async def test_list_messages_without_auth(service_client):
    """Test that listing messages requires authentication"""
    folder_id = 1
    
    response = await service_client.get(f'/api/v1/folders/{folder_id}/messages')
    assert response.status == 400


async def test_list_messages_empty_folder(service_client):
    """Test listing messages from empty folder"""
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
    
    # Create a folder
    folder_data = {'name': 'EmptyFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    response = await service_client.get(
        f'/api/v1/folders/{folder_id}/messages',
        headers=headers
    )
    assert response.status == 200
    
    json_response = response.json()
    assert isinstance(json_response, list)
    assert len(json_response) == 0


async def test_list_messages_user_isolation(service_client):
    """Test that users can only see messages in their own folders"""
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
    
    # Create folder for user 1
    folder1_data = {'name': 'User1Folder'}
    folder1_response = await service_client.post('/api/v1/folders', json=folder1_data, headers=headers1)
    folder1_id = folder1_response.json()['id']
    
    # Create message for user 1
    message_data = {
        'subject': 'User1Message',
        'body': 'Body',
        'recipient_email': 'recipient@example.com'
    }
    await service_client.post(
        f'/api/v1/folders/{folder1_id}/messages',
        json=message_data,
        headers=headers1
    )
    
    # Try to list messages for user 2 in user 1's folder
    response = await service_client.get(
        f'/api/v1/folders/{folder1_id}/messages',
        headers=headers2
    )
    
    # Should either return empty list or 403/404
    assert response.status in [200, 403, 404]
    
    if response.status == 200:
        messages = response.json()
        message_subjects = [m['subject'] for m in messages]
        assert 'User1Message' not in message_subjects


# ============================================================================
# Get Message by ID Tests
# ============================================================================

async def test_get_message_by_id_success(service_client):
    """Test getting a message by ID"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    # Create a message
    message_data = {
        'subject': 'Get Test Message',
        'body': 'This is a test message',
        'recipient_email': 'recipient@example.com'
    }
    
    create_response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    message_id = create_response.json()['id']
    
    # Get the message
    response = await service_client.get(
        f'/api/v1/messages/{message_id}',
        headers=headers
    )
    assert response.status == 200
    
    json_response = response.json()
    assert json_response['id'] == message_id
    assert json_response['subject'] == 'Get Test Message'
    assert json_response['body'] == 'This is a test message'
    assert json_response['recipient_email'] == 'recipient@example.com'


async def test_get_message_without_auth(service_client):
    """Test that getting a message requires authentication"""
    message_id = 1
    
    response = await service_client.get(f'/api/v1/messages/{message_id}')
    assert response.status == 400


async def test_get_message_not_found(service_client):
    """Test getting non-existent message"""
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
    
    message_id = 99999  # Non-existent message
    
    response = await service_client.get(
        f'/api/v1/messages/{message_id}',
        headers=headers
    )
    assert response.status == 404


async def test_get_message_user_isolation(service_client):
    """Test that users can only get their own messages"""
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
    
    # Create folder for user 1
    folder1_data = {'name': 'User1Folder'}
    folder1_response = await service_client.post('/api/v1/folders', json=folder1_data, headers=headers1)
    folder1_id = folder1_response.json()['id']
    
    # Create message for user 1
    message_data = {
        'subject': 'Private Message',
        'body': 'Secret content',
        'recipient_email': 'recipient@example.com'
    }
    
    create_response = await service_client.post(
        f'/api/v1/folders/{folder1_id}/messages',
        json=message_data,
        headers=headers1
    )
    message_id = create_response.json()['id']
    
    # Try to get message as user 2
    response = await service_client.get(
        f'/api/v1/messages/{message_id}',
        headers=headers2
    )
    
    # Should return 403 or 404
    assert response.status in [403, 404]


# ============================================================================
# Message Structure Tests
# ============================================================================

async def test_message_response_structure(service_client):
    """Test that message response has correct structure"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_data = {
        'subject': 'Structure Test',
        'body': 'Test body',
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    
    json_response = response.json()
    
    # Verify all expected fields are present
    assert 'id' in json_response
    assert 'subject' in json_response
    assert 'body' in json_response
    assert 'recipient_email' in json_response
    assert 'folder_id' in json_response
    assert 'created_at' in json_response
    
    # Verify types
    assert isinstance(json_response['id'], int)
    assert isinstance(json_response['subject'], str)
    assert isinstance(json_response['body'], str)
    assert isinstance(json_response['recipient_email'], str)
    assert isinstance(json_response['folder_id'], int)
    assert isinstance(json_response['created_at'], str)


async def test_list_messages_response_structure(service_client):
    """Test that list messages response has correct structure"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    response = await service_client.get(
        f'/api/v1/folders/{folder_id}/messages',
        headers=headers
    )
    
    json_response = response.json()
    
    # Verify response structure
    assert isinstance(json_response, list)
    
    # If there are messages, verify their structure
    if json_response:
        message = json_response[0]
        assert 'id' in message
        assert 'subject' in message
        assert 'body' in message
        assert 'recipient_email' in message
        assert 'folder_id' in message
        assert 'created_at' in message


# ============================================================================
# Edge Cases and Error Handling
# ============================================================================

async def test_create_message_with_long_content(service_client):
    """Test creating message with very long content"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    long_subject = 'A' * 500
    long_body = 'B' * 10000
    
    message_data = {
        'subject': long_subject,
        'body': long_body,
        'recipient_email': 'recipient@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    
    # Should either succeed or fail gracefully
    assert response.status in [201, 400]


async def test_create_message_with_special_characters(service_client):
    """Test creating message with special characters"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_data = {
        'subject': 'Test with émojis 🎉 and spëcial çhars!',
        'body': 'Body with <html> tags & special chars: @#$%^&*()',
        'recipient_email': 'recipient.test@example.com'
    }
    
    response = await service_client.post(
        f'/api/v1/folders/{folder_id}/messages',
        json=message_data,
        headers=headers
    )
    
    # Should either succeed or fail gracefully
    assert response.status in [201, 400]


async def test_create_multiple_messages_in_folder(service_client):
    """Test creating multiple messages in the same folder"""
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
    
    # Create a folder
    folder_data = {'name': 'TestFolder'}
    folder_response = await service_client.post('/api/v1/folders', json=folder_data, headers=headers)
    folder_id = folder_response.json()['id']
    
    message_count = 10
    message_ids = []
    
    for i in range(message_count):
        message_data = {
            'subject': f'Message {i}',
            'body': f'Body {i}',
            'recipient_email': 'recipient@example.com'
        }
        
        response = await service_client.post(
            f'/api/v1/folders/{folder_id}/messages',
            json=message_data,
            headers=headers
        )
        assert response.status == 201
        message_ids.append(response.json()['id'])
    
    # Verify all messages were created with unique IDs
    assert len(set(message_ids)) == message_count
    
    # Verify all messages are in the folder
    list_response = await service_client.get(
        f'/api/v1/folders/{folder_id}/messages',
        headers=headers
    )
    messages = list_response.json()
    assert len(messages) >= message_count


async def test_get_message_with_invalid_id_format(service_client):
    """Test getting message with invalid ID format"""
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
    
    response = await service_client.get('/api/v1/messages/invalid', headers=headers)
    assert response.status == 400


async def test_list_messages_with_invalid_folder_id(service_client):
    """Test listing messages with invalid folder ID format"""
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
    
    response = await service_client.get('/api/v1/folders/invalid/messages', headers=headers)
    assert response.status == 400
