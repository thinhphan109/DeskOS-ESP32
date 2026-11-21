import time
from PySide6.QtCore import QRectF
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import QWidget
from telemetry_history import TelemetryHistory


class TelemetryChart(QWidget):
    def __init__(self,parent=None):
        super().__init__(parent)
        self.history=TelemetryHistory()
        self.setMinimumHeight(190)
        self.setAccessibleName("CPU, GPU và RAM trong 60 giây · khoảng trống là dữ liệu thiếu")

    def add(self,data,host=""):
        self.history.add(data,host)
        self.update()

    def paintEvent(self,event):
        painter=QPainter(self); painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        now=time.monotonic(); rows=self.history.visible(now)
        left,top,right,bottom=34,38,self.width()-12,self.height()-26
        painter.setPen(QColor("#405448"))
        mode="DEMO" if self.history.source and self.history.source[1] else "THỰC"
        status="đang chờ dữ liệu" if not rows else "gián đoạn" if now-rows[-1][0]>2.5 else "1 mẫu/giây"
        painter.drawText(0,16,f"PC · 60 giây · {mode} · {status}")
        painter.setPen(QColor("#dadfd6"))
        for value in (0,50,100):
            y=bottom-(bottom-top)*value/100
            painter.drawLine(left,int(y),right,int(y))
            painter.setPen(QColor("#52645a"));painter.drawText(0,int(y)+4,str(value));painter.setPen(QColor("#dadfd6"))
        painter.setPen(QColor("#52645a"));painter.drawText(left,self.height()-5,"−60 s");painter.drawText(right-36,self.height()-5,"hiện tại")
        for index,(name,color) in enumerate((("CPU","#b25242"),("GPU","#65539a"),("RAM","#28654b"))):
            painter.setPen(QPen(QColor(color),2))
            painter.drawText(max(left,right-230)+index*78,16,name)
            for segment in self.history.segments(index,now):
                path=QPainterPath()
                for i,(timestamp,value) in enumerate(segment):
                    x=left+(right-left)*(timestamp-now+60)/60;y=bottom-(bottom-top)*value/100
                    if i==0: path.moveTo(x,y)
                    else: path.lineTo(x,y)
                painter.drawPath(path)
                if len(segment)==1:
                    timestamp,value=segment[0];painter.drawEllipse(QRectF(left+(right-left)*(timestamp-now+60)/60-1,bottom-(bottom-top)*value/100-1,2,2))
