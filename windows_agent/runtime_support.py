"""Local diagnostics and release self-check. No keys, track names or network writes."""
import importlib
import json
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path
import platform
import sys
import threading
from app_version import VERSION


def configure_logging(root):
    try:
        folder=Path(root)/"logs";folder.mkdir(parents=True,exist_ok=True)
        handler=RotatingFileHandler(folder/"companion.log",maxBytes=256*1024,backupCount=2,encoding="utf8")
        handler.setFormatter(logging.Formatter("%(asctime)s %(levelname)s %(message)s"))
        logger=logging.getLogger("mimi.desk");logger.setLevel(logging.INFO)
        if not logger.handlers:logger.addHandler(handler)
        def exception(kind,value,tb):
            # Avoid exception messages/locals: network and settings failures may contain secrets.
            location="unknown"
            while tb is not None:
                location=f"{Path(tb.tb_frame.f_code.co_filename).name}:{tb.tb_lineno}";tb=tb.tb_next
            logger.error("unhandled type=%s location=%s",kind.__name__,location)
        sys.excepthook=exception
        threading.excepthook=lambda args:exception(args.exc_type,args.exc_value,args.exc_traceback)
        logger.info("start version=%s frozen=%s",VERSION,bool(getattr(sys,"frozen",False)))
    except OSError:
        pass  # A locked/read-only profile must not prevent opening the app.


def diagnostics(*, check_media=False):
    from PySide6.QtCore import qVersion
    result={"schema":1,"app_version":VERSION,"python_version":platform.python_version(),
            "platform":sys.platform,"architecture":platform.machine(),"qt_version":qVersion(),
            "frozen":bool(getattr(sys,"frozen",False)),"dependencies":{},"assets":{}}
    for name in ("psutil","PIL.Image","PySide6.QtSvg","keyring.backends.Windows",
                 "winrt.windows.media.control","winrt.windows.media","winrt.windows.storage.streams"):
        try:importlib.import_module(name);result["dependencies"][name]="available"
        except Exception as error:result["dependencies"][name]=type(error).__name__
    for name in ("healthy.png","paused.png","music.png","shutdown.png"):
        result["assets"][name]=(Path(__file__).resolve().parent/"assets"/name).is_file()
    if check_media:
        import media_provider
        song=media_provider.snapshot()
        pixels=media_provider.artwork_rgb565(song)
        result["media"]={"available":bool(song.get("available")),"playing":bool(song.get("playing")),
                         "reason":song.get("reason"),"artwork_bytes":len(pixels) if pixels else 0}
    result["ok"]=all(v=="available" for v in result["dependencies"].values()) and all(result["assets"].values())
    return result


def write_report(path, report):
    target=Path(path);target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf8")


def self_check(path):
    """Can run from a packaged EXE; never loads saved device settings."""
    from PySide6.QtWidgets import QApplication
    from PySide6.QtGui import QPixmap
    from PySide6.QtCore import QTimer
    app=QApplication.instance() or QApplication([])
    app.setQuitOnLastWindowClosed(False)
    result=diagnostics()
    complete=threading.Event()
    def media_check():
        try:
            import media_provider
            song=media_provider.snapshot();pixels=media_provider.artwork_rgb565(song)
            result["media"]={"available":bool(song.get("available")),"playing":bool(song.get("playing")),
                             "reason":song.get("reason"),"artwork_bytes":len(pixels) if pixels else 0}
        finally:complete.set()
    worker=threading.Thread(target=media_check,name="MimiSelfCheck",daemon=True);worker.start()
    poll=QTimer();poll.timeout.connect(lambda:app.quit() if complete.is_set() else None);poll.start(25)
    timeout=QTimer();timeout.setSingleShot(True);timeout.timeout.connect(app.quit);timeout.start(12000)
    app.exec();poll.stop();timeout.stop()
    result["media_check_finished"]=complete.is_set()
    result["ok"]=result["ok"] and complete.is_set()
    from ui_theme import nav_icon
    image=QPixmap(str(Path(__file__).resolve().parent/"assets"/"healthy.png"))
    result["qt_render"]={"asset_loaded":not image.isNull(),"svg_loaded":not nav_icon("music").isNull()}
    result["ok"]=result["ok"] and all(result["qt_render"].values())
    write_report(path,result)
    return 0 if result["ok"] else 1


def preview_smoke(path):
    """Exercise packaged pages without loading settings or acquiring a live-agent lock."""
    from PySide6.QtWidgets import QApplication
    app=QApplication.instance() or QApplication([])
    from companion import MainWindow
    window=MainWindow(preview=True,start_services=False)
    window.show()
    sizes=[]
    try:
        for width,height in ((850,560),(1120,760),(1440,960)):
            window.resize(width,height)
            for index in range(window.stack.count()):
                window.set_tab(index)
                for _ in range(6):app.processEvents()
                sizes.append({"width":width,"height":height,"page":index,
                              "horizontal_scroll":window.content_scroll.horizontalScrollBar().maximum()})
        window.resize(1120,840);window.set_tab(0)
        for _ in range(6):app.processEvents()
        window.grab().save(str(Path(path).with_suffix(".png")))
        report={"schema":1,"version":VERSION,"frozen":bool(getattr(sys,"frozen",False)),
                "preview":True,"device_worker_started":window.worker is not None,"sizes":sizes,
                "ok":window.worker is None and all(row["horizontal_scroll"]==0 for row in sizes)}
        write_report(path,report)
        return 0 if report["ok"] else 1
    finally:window.quit_app()
