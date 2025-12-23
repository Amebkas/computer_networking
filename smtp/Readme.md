## Сборка:
```
cd build && cmake .. && make && cd ../
```

## Запуск сервера:
```
python3 -m smtpd -n -c DebuggingServer localhost:1025
```

## Запуск клиента:
```
./build/SmtpClient
```