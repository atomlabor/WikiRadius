var currentLocale = 'en';
var targetLat = 0, targetLon = 0;

function sendStatus(msg) {
  Pebble.sendAppMessage({'MESSAGE_KEY_STATUS': msg});
}

function getDistance(lat1, lon1, lat2, lon2) {
  var R = 6371000;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
          Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
          Math.sin(dLon/2) * Math.sin(dLon/2);
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}

function getBearing(lat1, lon1, lat2, lon2) {
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var y = Math.sin(dLon) * Math.cos(lat2 * Math.PI / 180);
  var x = Math.cos(lat1 * Math.PI / 180) * Math.sin(lat2 * Math.PI / 180) -
          Math.sin(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.cos(dLon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
}

function fetchWiki(lat, lon) {
  Pebble.sendAppMessage({'MESSAGE_KEY_STATUS': 'Searching...'});
  
  var url = 'https://' + currentLocale + '.wikipedia.org/w/api.php' +
    '?action=query&prop=coordinates|extracts&generator=geosearch' +
    '&ggscoord=' + lat + '|' + lon +
    '&ggsradius=200&ggslimit=1&exchars=1000&exintro&explaintext&format=json';

  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.timeout = 10000; 
  req.onload = function() {
    if (req.status === 200) {
      var res = JSON.parse(req.responseText);
      if (res.query && res.query.pages) {
        var page = res.query.pages[Object.keys(res.query.pages)[0]];
        if (page.coordinates) {
          targetLat = page.coordinates[0].lat;
          targetLon = page.coordinates[0].lon;
          Pebble.sendAppMessage({
            'MESSAGE_KEY_TITLE': page.title,
            'MESSAGE_KEY_SNIPPET': page.extract || "",
            'MESSAGE_KEY_DISTANCE': Math.round(getDistance(lat, lon, targetLat, targetLon)),
            'MESSAGE_KEY_BEARING': Math.round(getBearing(lat, lon, targetLat, targetLon))
          });
        }
      } else {
        sendStatus('Nothing around');
      }
    }
  };
  req.ontimeout = function() { sendStatus('Timeout Error'); };
  req.send(null);
}

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.MESSAGE_KEY_READY === 1) {
    if (e.payload.MESSAGE_KEY_LOCALE) currentLocale = e.payload.MESSAGE_KEY_LOCALE.substring(0, 2).toLowerCase();
    navigator.geolocation.getCurrentPosition(function(pos) {
      fetchWiki(pos.coords.latitude, pos.coords.longitude);
    }, function(err) {
      sendStatus('GPS Error');
    }, { enableHighAccuracy: true, timeout: 15000 });
  }
});

Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({'MESSAGE_KEY_READY': 1});
});