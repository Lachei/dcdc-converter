# set terminal dumb size 200, 60; set autoscale;

plot 'test.data' using 1:2 w l t 'high side v' , \
		'' using 1:3 w l t 'bat cur kwh', \
		'' using 1:4 w l t 'bat cur amp', \
		'' using 1:5 w l t 'bat cur v', \
		'' using 1:6 w l t 'duty cycle', \
		'' using 1:7 w l t 'v should low', \
		'' using 1:8 w l t 'err', \
		'' using 1:9 w l t 'goal amp'

pause -1

