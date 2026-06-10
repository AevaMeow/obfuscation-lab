# Лабораторная работа по обфускации кода
# Запуск

Перейти в каталог проекта:

```bash
cd ~/obfuscation-lab
```

Собрать проект:

```bash
make clean
make
```

Запустить чистую версию:

```bash
./crackme_clean AAAA-BBBB-CCCC-DDDD
./crackme_clean K1R1-0BF5-VM42-C0DE
```

Запустить обфусцированную версию:

```bash
./crackme_ida AAAA-BBBB-CCCC-DDDD
./crackme_ida K1R1-0BF5-VM42-C0DE
```

Запустить solver с подробным выводом:

```bash
./solver
```

Вывести только восстановленный ключ:

```bash
./solver -q
```

Передать найденный ключ в crackme:

```bash
./crackme_ida "$(./solver -q)"
```

Проверить наличие открытых строк:

```bash
strings crackme_clean | grep -E "K1R1|Correct|Wrong|Usage"
strings crackme_ida | grep -E "K1R1|Correct|Wrong|Usage"
```

Пересоздать VM-bytecode:

```bash
make regenerate
```
