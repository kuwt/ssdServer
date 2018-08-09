import os
import math
import random
import numpy as np
import tensorflow as tf
import cv2

slim = tf.contrib.slim
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import sys
import time
# import RealSense
sys.path.append("C:\\Program Files (x86)\\Intel RealSense SDK 2.0\\bin\\x64")
import pyrealsense2 as rs

sys.path.append(r"D:\temp\Deep_Learning\SSD-Tensorflow")
from nets import ssd_vgg_300, ssd_common, np_methods
from preprocessing import ssd_vgg_preprocessing

import zmq
from objectInfos_pb2 import objectInfos

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")


class ssdServer:
    def __init__(self):
        self.num_classes = 32
        self.setupNetwork()
        print("Ready")
    def setupNetwork(self):
        # TensorFlow session: grow memory when needed. TF, DO NOT USE ALL MY GPU MEMORY!!!
        gpu_options = tf.GPUOptions(allow_growth=True)
        config = tf.ConfigProto(log_device_placement=False, gpu_options=gpu_options)
        self.isess = tf.InteractiveSession(config=config)
        # Input placeholder.
        net_shape = (300, 300)
        data_format = 'NHWC'
        self.img_input = tf.placeholder(tf.uint8, shape=(None, None, 3))
        # Evaluation pre-processing: resize to SSD net shape.
        image_pre, labels_pre, bboxes_pre,  self.bbox_img = ssd_vgg_preprocessing.preprocess_for_eval(
            self.img_input, None, None, net_shape, data_format, resize=ssd_vgg_preprocessing.Resize.WARP_RESIZE)
        self.image_4d = tf.expand_dims(image_pre, 0)

        # Define the SSD model.
        reuse = True if 'ssd_net' in locals() else None
        ssd_class = ssd_vgg_300.SSDNet
        ssd_params = ssd_class.default_params._replace(num_classes=self.num_classes)
        ssd_net = ssd_class(ssd_params)
        with slim.arg_scope(ssd_net.arg_scope(data_format=data_format)):
            self.predictions,self.localisations, _, _ = ssd_net.net(self.image_4d, is_training=False, reuse=reuse)

        # Restore SSD model.
        ckpt_filename = r'D:/temp1/tless_log7_p3/model.ckpt-43529'
        self.isess.run(tf.global_variables_initializer())
        saver = tf.train.Saver()
        saver.restore(self.isess, ckpt_filename)

        # SSD default anchor boxes.
        self.ssd_anchors = ssd_net.anchors(net_shape)

    def inference(self, img):
        rclasses, rscores, rbboxes = self.process_image(img, select_threshold=0.2, nms_threshold=0.1,
                                                   nms_within_same_class=False)

        return rclasses, rscores, rbboxes
        #for i, rclass in enumerate(rclasses):
        #    print("class = ", rclasses[i])
        #    print("score = ", rscores[i])
        #    print("box = ", rbboxes[i])

    # Main image processing routine.
    def process_image(self, img, select_threshold=0.01, nms_threshold=0.45, net_shape=(300, 300), nms_within_same_class=True):
        # Run SSD network.
        rimg, rpredictions, rlocalisations, rbbox_img = self.isess.run([ self.image_4d,  self.predictions, self.localisations, self.bbox_img],
                                                                  feed_dict={self.img_input: img})
        # print(rpredictions[2][0][1][1])
        # Get classes and bboxes from the net outputs.
        rclasses, rscores, rbboxes = np_methods.ssd_bboxes_select(
            rpredictions, rlocalisations, self.ssd_anchors,
            select_threshold=select_threshold, img_shape=net_shape, num_classes=self.num_classes, decode=True)

        rbboxes = np_methods.bboxes_clip(rbbox_img, rbboxes)

        rclasses, rscores, rbboxes = np_methods.bboxes_sort(rclasses, rscores, rbboxes, top_k=400)
        rclasses, rscores, rbboxes = np_methods.bboxes_nms(rclasses, rscores, rbboxes,
                                                           nms_threshold=nms_threshold,
                                                           within_same_class=nms_within_same_class)

        # Resize bboxes to original image shape. Note: useless for Resize.WARP!
        rbboxes = np_methods.bboxes_resize(rbbox_img, rbboxes)
        return rclasses, rscores, rbboxes

if __name__ == '__main__' :
    ser = ssdServer()


    while True:
        print("waiting connection")
        message = socket.recv()
        print("message recv")
        if (message):
            # decode
            nparr = np.fromstring(message, np.uint8)
            img_decode = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            print(type(img_decode), " ", img_decode.shape)
            #path = r"D:\temp\Deep_Learning\SSD-Tensorflow\demo_own_gray\01.bmp"
            #img_decode = cv2.imread(path)
            #print(type(img_decode)," ",img_decode.shape)
            rclasses, rscores, rbboxes = ser.inference(img_decode)
            height, width = img_decode.shape[:2]

            #scale back rbboxes
            for rbbox in rbboxes:
                rbbox[0] = rbbox[0] * height
                rbbox[1] = rbbox[1] * width
                rbbox[2] = rbbox[2] * height
                rbbox[3] = rbbox[3] * width

        # Send reply back to client
        stObjectInfos = objectInfos()
        for i, rclass in enumerate(rclasses):
            info = stObjectInfos.infos.add()
            info.objClass = rclasses[i]
            info.score = rscores[i]
            info.tly = int(rbboxes[i][0])
            info.tlx = int(rbboxes[i][1])
            info.bry = int(rbboxes[i][2])
            info.brx = int(rbboxes[i][3])


        str = stObjectInfos.SerializeToString()
        socket.send(str)