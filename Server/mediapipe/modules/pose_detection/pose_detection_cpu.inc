"\n\202\001\022\027ImageToTensorCalculator\032\013IMAGE:image\"\025TENSORS:inpu"
"t_tensors\"#LETTERBOX_PADDING:letterbox_padding:\036\232\325\276\373\t\030\010"
"\340\001\020\340\001\030\001\"\n\r\000\000\200\277\025\000\000\200?(\0020\001"
"\n\213\001\022\023InferenceCalculator\032\025TENSORS:input_tensors\"\031TENSOR"
"S:detection_tensors:B\272\237\335\204\n<\n6mediapipe/modules/pose_detection/po"
"se_detection.tflite*\002\"\000\nT\022\024SsdAnchorsCalculator2\007anchors:3"
"\372\307\233\257\007-\010\340\001\020\340\001\035\000\000\030>%\000\000@?-\000"
"\000\000?5\000\000\000?8\005P\010P\020P P P ]\000\000\200?p\001\n\250\001\022"
"\035TensorsToDetectionsCalculator\032\031TENSORS:detection_tensors\" DETECTION"
"S:unfiltered_detections*\017ANCHORS:anchors:9\372\352\340\200\n3\010\001\020"
"\316\021\030\014%\000\000`C-\000\000`C5\000\000`C=\000\000`CH\004P\004X\002`"
"\000p\001x\001\205\001\000\000\310B\235\001\000\000\000?\nZ\022\033NonMaxSuppr"
"essionCalculator\032\025unfiltered_detections\"\023filtered_detections:\017"
"\342\303\242\323\001\t\035\232\231\231> \0038\001\n\201\001\022#DetectionLette"
"rboxRemovalCalculator\032\036DETECTIONS:filtered_detections\032#LETTERBOX_PADD"
"ING:letterbox_padding\"\025DETECTIONS:detectionsR\013IMAGE:imagez\025DETECTION"
"S:detections\242\001\020PoseDetectionCpu"
