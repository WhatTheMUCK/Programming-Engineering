workspace "Email Service" "C1 + C2 + Dynamic views for Outlook-like email backend (C++/userver)" {
  !identifiers hierarchical

  model {
    user = person "Пользователь" "Работает с почтой: папки и письма."

    idp = softwareSystem "Identity Provider" "Внешняя аутентификация (OAuth2/OIDC)." {
      tags "External"
    }

    smtpProvider = softwareSystem "External Mail Servers" "Внешние SMTP-серверы для доставки писем." {
      tags "External"
    }

    email = softwareSystem "Email Service" "Backend API для пользователей, папок и сообщений." {

      web = container "Web Client" "Веб-интерфейс (SPA), вызывает backend API." "Web Application"

      apiGateway = container "API Gateway" "Единая точка входа: TLS, роутинг, rate limit." "Nginx (Reverse Proxy)"

      userService = container "User Service" "Создание и поиск пользователей." "C++ (Yandex userver), REST API"
      folderService = container "Folder Service" "Создание папок и получение списка папок." "C++ (Yandex userver), REST API"
      messageService = container "Message Service" "Создание/получение писем." "C++ (Yandex userver), REST API"

      outboxQueue = container "Outbox Queue" "Очередь событий для асинхронной отправки писем." "RabbitMQ (AMQP)" {
        tags "Optional"
      }

      smtpWorker = container "SMTP Worker" "Воркер: забирает события и отправляет письма наружу по SMTP." "C++ worker" {
        tags "Optional"
      }

      db = container "Database" "Хранит пользователей, папки, сообщения." "PostgreSQL" {
        tags "Database"
      }
    }

    user -> email.web "UI / CLI" "HTTPS"
    email.web -> email.apiGateway "Вызовы API" "HTTPS/REST"

    email.apiGateway -> idp "JWT/JWK или introspection" "OAuth2/OIDC"

    email.apiGateway -> email.userService "REST" "HTTPS/REST"
    email.apiGateway -> email.folderService "REST" "HTTPS/REST"
    email.apiGateway -> email.messageService "REST" "HTTPS/REST"

    email.userService -> email.db "users" "SQL"
    email.folderService -> email.db "folders" "SQL"
    email.messageService -> email.db "messages" "SQL"

    email.messageService -> email.outboxQueue "MessageCreated (опц.)" "AMQP"
    email.smtpWorker -> email.outboxQueue "Consume events (опц.)" "AMQP"
    email.smtpWorker -> smtpProvider "Send email" "SMTP"
  }

  views {
    themes default

    systemContext email "C1" "System Context" {
      include user
      include email
      include idp
      include smtpProvider
      autolayout lr
    }

    container email "C2" "Container diagram" {
      include *
      autolayout lr
    }

    dynamic email "D1_CreateUser" "API: Создание нового пользователя" {
      autolayout lr
      1: user -> email.web "Заполняет регистрацию" "HTTPS"
      2: email.web -> email.apiGateway "POST /api/v1/users" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена (если требуется)" "OAuth2/OIDC"
      4: email.apiGateway -> email.userService "Создать пользователя" "HTTPS/REST"
      5: email.userService -> email.db "INSERT user" "SQL"
      6: email.userService -> email.apiGateway "201 Created + userId" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D2_FindUserByLogin" "API: Поиск пользователя по логину" {
      autolayout lr
      1: user -> email.web "Ищет пользователя по логину" "HTTPS"
      2: email.web -> email.apiGateway "GET /api/v1/users/by-login?login=..." "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.userService "Найти по логину" "HTTPS/REST"
      5: email.userService -> email.db "SELECT user WHERE login=..." "SQL"
      6: email.userService -> email.apiGateway "200 OK (user)" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D3_SearchUserByNameMask" "API: Поиск пользователя по маске имя+фамилия" {
      autolayout lr
      1: user -> email.web "Ищет по маске имени/фамилии" "HTTPS"
      2: email.web -> email.apiGateway "GET /api/v1/users/search?mask=..." "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.userService "Поиск по маске" "HTTPS/REST"
      5: email.userService -> email.db "SELECT ... WHERE full_name ILIKE mask" "SQL"
      6: email.userService -> email.apiGateway "200 OK (list)" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D4_CreateFolder" "API: Создание новой почтовой папки" {
      autolayout lr
      1: user -> email.web "Создает папку" "HTTPS"
      2: email.web -> email.apiGateway "POST /api/v1/folders" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.folderService "Создать папку" "HTTPS/REST"
      5: email.folderService -> email.db "INSERT folder" "SQL"
      6: email.folderService -> email.apiGateway "201 Created + folderId" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D5_ListFolders" "API: Получение перечня всех папок" {
      autolayout lr
      1: user -> email.web "Открывает список папок" "HTTPS"
      2: email.web -> email.apiGateway "GET /api/v1/folders" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.folderService "Список папок" "HTTPS/REST"
      5: email.folderService -> email.db "SELECT folders WHERE userId=..." "SQL"
      6: email.folderService -> email.apiGateway "200 OK (list)" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D6_CreateMessageInFolder" "API: Создание нового письма в папке (+ отправка наружу, опционально)" {
      autolayout lr
      1: user -> email.web "Пишет письмо и нажимает Отправить/Сохранить" "HTTPS"
      2: email.web -> email.apiGateway "POST /api/v1/folders/{folderId}/messages" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.messageService "Создать письмо" "HTTPS/REST"
      5: email.messageService -> email.db "INSERT message" "SQL"
      6: email.messageService -> email.outboxQueue "Publish MessageCreated (опц.)" "AMQP"
      7: email.smtpWorker -> email.outboxQueue "Consume MessageCreated (опц.)" "AMQP"
      8: email.smtpWorker -> smtpProvider "SMTP SEND (опц.)" "SMTP"
      9: email.messageService -> email.apiGateway "201 Created + messageId" "HTTPS/REST"
      10: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D7_ListMessagesInFolder" "API: Получение всех писем в папке" {
      autolayout lr
      1: user -> email.web "Открывает папку" "HTTPS"
      2: email.web -> email.apiGateway "GET /api/v1/folders/{folderId}/messages" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.messageService "Список писем" "HTTPS/REST"
      5: email.messageService -> email.db "SELECT messages WHERE folderId=..." "SQL"
      6: email.messageService -> email.apiGateway "200 OK (list)" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    dynamic email "D8_GetMessageById" "API: Получение письма по коду (id)" {
      autolayout lr
      1: user -> email.web "Открывает письмо" "HTTPS"
      2: email.web -> email.apiGateway "GET /api/v1/messages/{messageId}" "HTTPS/REST"
      3: email.apiGateway -> idp "Проверка токена" "OAuth2/OIDC"
      4: email.apiGateway -> email.messageService "Получить письмо" "HTTPS/REST"
      5: email.messageService -> email.db "SELECT message WHERE id=..." "SQL"
      6: email.messageService -> email.apiGateway "200 OK (message)" "HTTPS/REST"
      7: email.apiGateway -> email.web "Ответ" "HTTPS/REST"
    }

    styles {
      element "Person" {
        shape person
      }

      element "Database" {
        shape cylinder
      }

      element "External" {
        background #999999
        color #ffffff 
      }

      element "Optional" {
        opacity 60
      }
    }
  }
}
