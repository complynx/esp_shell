import sys

PWM_MAX=22222
with open("pwm.csv","w+") as f:
	f.write("0,")
	for i in range(1,254):
		d=pow(i/255.,2.5)*PWM_MAX
		if(d<i):
			d=i
		f.write("%d,"%(d))
		if i%20 == 0:
			f.write("\n");
	f.write("%d\n"%PWM_MAX)
