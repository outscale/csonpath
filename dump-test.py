import csonpath

o = csonpath.CsonPath("$.a")
#breakpoint()
o = csonpath.CsonPath("$.a")
d =  {"B": "Bh", "a": "le a"}
r = o.find_first(d)

print(r)

r = o.find_all(d)

print(r)

o = csonpath.CsonPath("$.C")
r = o.find_first(d)

print(r)

o = csonpath.CsonPath("$[*]")
r = o.find_all(d)

print(r)

d = { "B": {"a": "true A"}, "a": "le a", "C": {"B": "not good", "a": "oh" } }
o = csonpath.CsonPath("$[*].a")
r = o.find_all(d)
print(r)

o = csonpath.CsonPath("$..a")
r = o.find_all(d)
print(".. find")
print(r)

print(d)
o.remove(d)
print("after remove:")
print(d)

d["C"]["la"] = "lo"
print(d)
o = csonpath.CsonPath("$.C[*]")
o.remove(d)
print("after remove 2:")
print(d)

o = csonpath.CsonPath("$.C")
o.remove(d)
print("after remove 3:")
print(d)

d["ar"] = [1,2,3]

print(d)
o = csonpath.CsonPath("$.ar[1]")
o.remove(d)
print("after remove 4:")
print(d)

o = csonpath.CsonPath("$.ar[*]")
o.remove(d)
print("after remove 5:")
print(d)


#r["context"][0]['a'] = "a new one"
#print(r)
#print(d)
#del r["context"][0]['a']
#print(d)
