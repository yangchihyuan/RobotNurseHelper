# Export YOLOv11 pose model to ONNX format
# Import the YOLO class from the ultralytics package
# Need a python environment with ultralytics installed
#yoll11n-pose.pt model can be downloaded from https://ultralytics.com/models or https://huggingface.co/Ultralytics/YOLO11/tree/a01aaa06caeff788b052e193acb76b3f21571b3a

from ultralytics import YOLO 
# Load the pose model 
model = YOLO('yolo11n-pose.pt') 
# Export to ONNX format 
model.export(format='onnx', opset=17) 
