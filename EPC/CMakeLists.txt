cmake_minimum_required (VERSION 2.8.13)
set (targetName  Detect3DFace)
PROJECT(${targetName})

add_definitions(-msse2 -pthread -std=c++11 -fPIC -ffast-math)
add_definitions(${PCL_DEFINITIONS})

find_package(PCL 1.8 REQUIRED)
#find_package(OpenCV REQUIRED)
#find_package(dlib REQUIRED) 

message(STATUS "    version: ${OpenCV_VERSION}")

add_subdirectory(.. FaceRecn_build) 

include_directories(

    ../
)



add_executable(EPCViewer EPCViewer.cpp ../PCLShow/PointCloudViewer.cpp    ../protoServer/asioServer.cpp)
target_include_directories(EPCViewer PUBLIC  ${OpenCV_INCLUDE_DIRS} ${PCL_INCLUDE_DIRS} ${EIGEN_INCLUDE_DIRS})
target_link_libraries(EPCViewer FaceRecn EpcClassDL libdlib.a X11 pthread  glog  ${PCL_LIBRARIES} ${OpenCV_LIBRARIES})

add_executable(EPC_pose head_pose.cpp Net.cpp ../Filter/dataFilter.cpp)
target_include_directories(EPC_pose PUBLIC  ${OpenCV_INCLUDE_DIRS}  ${EIGEN_INCLUDE_DIRS} )
target_link_libraries(EPC_pose  FaceRecn libdlib.a X11 pthread   ${OpenCV_LIBRARIES})

include_directories(${PCL_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
add_library(EpcClassDL SHARED  EpcClassDL.cpp Net.cpp)
