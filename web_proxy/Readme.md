## Сборка:
```
cd build && cmake .. && make && cd ../
```

## Запуск:
`./build/http_proxy`

## Описание алгоритма кэширования
1. Извлечение ключа: При получении GET-запроса прокси-сервер парсит URL и извлекает имя хоста и путь к ресурсу. Полная строка URL (хост + путь) используется как уникальный идентификатор ресурса.
2. Генерация имени файла: Чтобы сохранить данные на диск, прокси преобразует URL в имя файла, заменяя спецсимволы (например, `/`, `:`, `?`) на подчеркивания. Все файлы хранятся в директории `cache/`.
2. Проверка наличия кэша: Программа проверяет существование файла с помощью `access()`.
  - Если файл существует, прокси открывает его, считывает содержимое и отправляет клиенту без обращения к целевому серверу.
  - Если файла нет, прокси устанавливает TCP-соединение с целевым сервером (порт 80).
3. Сохранение и трансляция: При "промахе" прокси получает данные от сервера порциями. Каждая порция одновременно отправляется клиенту и записывается в файл кэша. Таким образом, при следующем запросе данные будут отданы мгновенно с диска.

## Пример работы:
I: `./build/http_proxy`

### Впервые делаем запрос на некий url
I: `curl -x http://localhost:8888 http://www.example.com/index.html`

O:
```
HTTP Proxy Server started on port 8888
[CACHE MISS] Fetching www.example.com/index.html from server
[INFO] Current Stats -> Hits: 0, Misses: 1
```

### Запрос несуществующего хоста
I: `curl -x http://localhost:8888 http://unknown.host.site/index.html`

O:
```
[CACHE MISS] Fetching unknown.host.site/index.html from server
[ERROR] Host not found: unknown.host.site
[INFO] Current Stats -> Hits: 0, Misses: 2
```

### Повтороный запрос на некий url
I: `curl -x http://localhost:8888 http://www.example.com/index.html`

O:
```
[CACHE HIT] Serving www.example.com/index.html from disk
[INFO] Current Stats -> Hits: 1, Misses: 2
```