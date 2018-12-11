rm -rf file_test.dat ATTR_TANK.dat cache.dat mem_corruption.dat mem_correction.dat && \
touch file_test.dat ATTR_TANK.dat cache.dat mem_corruption.dat mem_correction.dat && \
g++ app.cpp test.cpp -o app --std=c++11 && ./app && \
rm -rf file_test.dat ATTR_TANK.dat cache.dat mem_corruption.dat mem_correction.dat
