workspace "Email Service" "C1+C2+D1 for Outlook-like email system (C++/Yandex userver)" {

    !identifiers hierarchical

    model {
        user = person "Пользователь" "Читает и отправляет письма, управляет папками."

        admin = person "Администратор" "Поддержка и администрирование сервиса. (Опционально)" {
            tags "Admin"
        }

        idp = softwareSystem "Identity Provider" "Внешняя аутентификация пользователей (OAuth2/OIDC)." {
            tags "External"
        }

        externalMail = softwareSystem "External Mail Servers" "Внешние почтовые серверы/домены (SMTP/IMAP)." {
            tags "External"
        }

        mailSystem = softwareSystem "Почтовый сервис" "Backend API для управления пользователями, папками и сообщениями." {

            webClient = container "Web Client" "Веб-интерфейс для работы с почтой." "Web Application"

            apiGateway = container "API Gateway" "Единая точка входа: маршрутизация, rate-limit, TLS termination." "Nginx (Reverse Proxy)" {
                tags "Optional"
            }

            userService = container "User Service" "Создание и поиск пользователей." "C++ (Yandex userver, REST API)"

            folderService = container "Folder Service" "Создание папок и получение списка папок." "C++ (Yandex userver, REST API)"

            messageService = container "Message Service" "Создание и получение писем." "C++ (Yandex userver, REST API)"

            messageBroker = container "Message Broker" "Асинхронные события (опционально)." "RabbitMQ (AMQP)" {
                tags "Optional"
            }

            mailGateway = container "Mail Gateway" "Адаптер для отправки/получения почты через SMTP/IMAP (опционально)." "C++ (Yandex userver worker)" {
                tags "Optional"
            }

            db = container "Database" "Хранит пользователей, папки, сообщения." "PostgreSQL" {
                tags "Database"
            }
        }

        user -> mailSystem "Работает с почтой" "HTTPS"
        admin -> mailSystem "Администрирует/поддерживает" "HTTPS"

        mailSystem -> idp "Аутентифицирует пользователя" "OAuth2/OIDC"
        mailSystem -> externalMail "Отправляет/получает письма" "SMTP/IMAP"

        user -> mailSystem.webClient "Работает с почтой" "HTTPS"
        admin -> mailSystem.apiGateway "Администрирует/поддерживает" "HTTPS"

        mailSystem.webClient -> mailSystem.apiGateway "Вызовы API" "HTTPS/JSON"

        mailSystem.apiGateway -> mailSystem.userService "Запросы пользователей" "HTTPS/JSON"
        mailSystem.apiGateway -> mailSystem.folderService "Запросы папок" "HTTPS/JSON"
        mailSystem.apiGateway -> mailSystem.messageService "Запросы писем" "HTTPS/JSON"

        mailSystem.apiGateway -> idp "Валидация токена" "OAuth2/OIDC"

        mailSystem.userService -> mailSystem.db "CRUD users" "PostgreSQL protocol"
        mailSystem.folderService -> mailSystem.db "CRUD folders" "PostgreSQL protocol"
        mailSystem.messageService -> mailSystem.db "CRUD messages" "PostgreSQL protocol"

        mailSystem.messageService -> mailSystem.messageBroker "Публикует событие о создании письма" "AMQP"
        mailSystem.mailGateway -> mailSystem.messageBroker "Читает события на отправку" "AMQP"
        mailSystem.mailGateway -> externalMail "Отправка/получение писем" "SMTP/IMAP"
    }

    views {
        systemContext mailSystem "C1" "System Context" {
            include user
            include admin
            include mailSystem
            include idp
            include externalMail
            autolayout lr
        }

        container mailSystem "C2" "Container diagram" {
            include *
            autolayout lr
        }

        dynamic mailSystem "D1" "Создание письма и отправка наружу" {
            1: user -> mailSystem.webClient "Пишет письмо и нажимает Отправить" "HTTPS"
            2: mailSystem.webClient -> mailSystem.apiGateway "POST /folders/{id}/messages" "HTTPS/JSON"
            3: mailSystem.apiGateway -> idp "Валидация access token" "OAuth2/OIDC"
            4: mailSystem.apiGateway -> mailSystem.messageService "Создать письмо в папке" "HTTPS/JSON"
            5: mailSystem.messageService -> mailSystem.db "Сохранить письмо" "PostgreSQL protocol"
            6: mailSystem.messageService -> mailSystem.messageBroker "Опубликовать событие MessageCreated" "AMQP"
            7: mailSystem.mailGateway -> mailSystem.messageBroker "Получить событие на отправку" "AMQP"
            8: mailSystem.mailGateway -> externalMail "Отправить письмо" "SMTP/IMAP"

            autolayout lr
        }

        styles {
            element "Person" {
                shape person
                background #08427b
                color #ffffff
            }

            element "Admin" {
                background #6b3fa0
                color #ffffff
            }

            element "Software System" {
                shape roundedbox
                background #1168bd
                color #ffffff
            }

            element "Container" {
                shape roundedbox
                background #2a7bd6
                color #ffffff
            }

            element "Database" {
                shape cylinder
                background #0b3d91
                color #ffffff
            }

            element "External" {
                background #999999
                color #ffffff
            }

            element "Optional" {
                background #6b8fb3
                color #ffffff
            }
        }
    }
}
