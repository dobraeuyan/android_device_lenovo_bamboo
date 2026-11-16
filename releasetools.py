import common

def _add_splash(info):
  input_zip = getattr(info, "input_zip", None) or getattr(info, "target_zip", None)
  if input_zip is None:
    return
  try:
    data = input_zip.read("RADIO/splash.img")
  except KeyError:
    return
  common.ZipWriteStr(info.output_zip, "splash.img", data)
  info.script.Print("Flashing splash image unconditionally...")
  info.script.AppendExtra('package_extract_file("splash.img", "/dev/block/by-name/splash");')

def FullOTA_InstallEnd(info):
  _add_splash(info)

def IncrementalOTA_InstallEnd(info):
  _add_splash(info)
