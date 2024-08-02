#cloud:cloud.cpp bundle.cpp -lpthread


# 伪对象
.PHONY:cloud
cloud:cloud.cpp 
	g++ $^ -o $@ -lstdc++fs -ljsoncpp -lbundle -Llib -lpthread -Iinclude -g

.PHONY:clean
clean:
	rm -rf cloud
