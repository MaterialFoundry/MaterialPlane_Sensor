const char updatePage[] PROGMEM = R"---(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta http-equiv="Content-type" content="text/html; charset=utf-8">
    <link rel="icon" type="image/x-icon" href="favicon.ico">
    <title>Update</title>
    <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
  </head>

  <style>
    :root {
      margin: 0; 
      padding: 0; 
      background-color: black; 
      width: 600px;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol";
      font-size: 12pt;
      color: white;
    }
  </style>

  <form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
    <div>
      Material Sensor
    </div>
    
    <br>
    
    <div>
      Error: Could not find webserver files.<br>Please follow <a href="https://github.com/MaterialFoundry/MaterialPlane_Sensor/wiki/Firmware-Update#updating-the-sensor-webserver">these</a> instructions to continue.
    <div>
    
    <br>
    
    <div>
      File upload:
    </div>
    
    <input type='file' name='update' id='file' onchange='sub(this)' style=display:none>
    <label id='file-input' for='file'>
      Choose file...
    </label>
    <input type='submit' class=btn value='Update'>

    <div id='prg'></div>
    <br>
    <div id='prgbar'>
      <div id='bar'></div>
    </div>
    <br>
    
  </form>

  <script>
    function sub(obj){
        var fileName = obj.value.split('\\\\');
        document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];
    };
    $('form').submit(function(e){
        e.preventDefault();
        var form = $('#upload_form')[0];
        var data = new FormData(form);
        $.ajax({
            url: '/update',
            type: 'POST',
            data: data,
            contentType: false,
            processData:false,
            xhr: function() {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', function(evt) {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        $('#prg').html('progress: ' + Math.round(per*100) + '%');
                        $('#bar').css('width',Math.round(per*100) + '%');
                    }
                }, false);
                return xhr;
            },
            success:function(d, s) {
                console.log('success!') 
            },
            error: function (a, b, c) {}
        });
    });
</script>
)---";
