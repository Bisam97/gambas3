#!/usr/bin/python

str = ''
i = 0

for i in range(0, 1000000):
	for j in range(65, 91):
		c = chr(j)
		str += '.(' + c + ')'

print(len(str))
