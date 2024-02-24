#cloud:cloud.cpp bundle.cpp -lpthread


# 伪对象
.PHONY:cloud
cloud:cloud.cpp 
	g++ -g $^ -o $@ -lstdc++fs -ljsoncpp -lbundle -Llib -lpthread -Iinclude

.PHONY:clean
clean:
	rm -rf cloud
