"\nf\022\027ImageToTensorCalculator\032\013IMAGE:image\032\rNORM_RECT:roi\"\025"
"TENSORS:input_tensors:\030\232\325\276\373\t\022\010\300\001\020\300\001\"\n\r"
"\000\000\000\000\025\000\000\200?\nF\022\030FaceLandmarksModelLoader*\035WITH_"
"ATTENTION:with_attention2\013MODEL:model\n;\022 TfLiteCustomOpResolverCalculat"
"or2\027OP_RESOLVER:op_resolver\nv\022\023InferenceCalculator\032\025TENSORS:in"
"put_tensors\"\026TENSORS:output_tensors*\013MODEL:model*\027OP_RESOLVER:op_res"
"olver:\n\272\237\335\204\n\004*\002\"\000\n\312\001\022\017SwitchContainer\032"
"\016output_tensors\"\020landmark_tensors\"\020face_flag_tensor*\025ENABLE:with"
"_attention:l\222\326\341\247\nf\0221\022\033SplitTensorVectorCalculator:\022"
"\362\350\326\335\007\014\n\004\010\000\020\001\n\004\010\001\020\002\0221\022"
"\033SplitTensorVectorCalculator:\022\362\350\326\335\007\014\n\004\010\000\020"
"\006\n\004\010\006\020\007\nZ\022\031TensorsToFloatsCalculator\032\030TENSORS:"
"face_flag_tensor\"\031FLOAT:face_presence_score:\010\332\226\254\236\n\002\010"
"\001\nX\022\026ThresholdingCalculator\032\031FLOAT:face_presence_score\"\022FL"
"AG:face_presence:\017\222\276\344\337\007\t\t\000\000\000\000\000\000\340?\nQ"
"\022\016GateCalculator\032\020landmark_tensors\032\023ALLOW:face_presence\""
"\030ensured_landmark_tensors\n\250\001\022\017SwitchContainer\032 TENSORS:ensu"
"red_landmark_tensors\"\023LANDMARKS:landmarks*\025ENABLE:with_attention:G\222"
"\326\341\247\nA\022\030\022\026TensorsToFaceLandmarks\022%\022#TensorsToFaceLa"
"ndmarksWithAttention\nf\022\034LandmarkProjectionCalculator\032\030NORM_LANDMA"
"RKS:landmarks\032\rNORM_RECT:roi\"\035NORM_LANDMARKS:face_landmarksR\013IMAGE:"
"imageR\007ROI:roiz\030LANDMARKS:face_landmarks\202\001\035WITH_ATTENTION:with_"
"attention\242\001\017FaceLandmarkCpu"
