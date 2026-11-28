import redis

r = redis.Redis(host="broker", decode_responses=True)
N = 12
TIMEOUT = 5
done = set()

r.delete("tasks", "results")

for i in range(1, N + 1):
    r.lpush("tasks", i)

while len(done) < N:
    item = r.brpop("results", timeout=TIMEOUT)
    if item is None:
        for i in range(1, N + 1):
            if i not in done:
                r.lpush("tasks", i)
        continue
    _, result = item
    n = int(result.split("^")[0])
    if n not in done:
        done.add(n)
        print(result)

r.delete("tasks", "results")
print("Все задачи выполнены")
