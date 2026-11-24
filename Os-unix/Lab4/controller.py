import redis

r = redis.Redis(host="broker", decode_responses=True)
N = 12

for i in range(1, N + 1):          # кладём задачи в очередь
    r.lpush("tasks", i)

for _ in range(N):                 # собираем результаты
    _, result = r.brpop("results")
    print(result)
