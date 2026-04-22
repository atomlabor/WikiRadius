var currentLocale = 'en';
var lastFetchLat = 0;
var lastFetchLon = 0;
var targetLat = 0;
var targetLon = 0;
var isTracking = false;

const FETCH_THRESHOLD = 150; 

function sendStatus(msg) {
  Pebble.sendAppMessage({'MESSAGE_KEY_STATUS': msg});
}

function getDistance(lat1, lon1, lat2, lon2) {
  var R = 6371000;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var a = Math.sin(dLat/2) * Math.sin(dLat/2) + Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.sin(dLon/2) * Math.sin(dLon/2);
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
}

function getBearing(lat1, lon1, lat2, lon2) {
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var y = Math.sin(dLon) * Math.cos(lat2 * Math.PI / 180);
  var x = Math.cos(lat1 * Math.PI / 180) * Math.sin(lat2 * Math.PI / 180) - Math.sin(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.cos(dLon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
}

function fetchWiki(lat, lon) {
  lastFetchLat = lat;
  lastFetchLon = lon;
  sendStatus("Searching Wiki...");
  
  var url = 'https://' + currentLocale + '.wikipedia.org/w/api.php?action=query&prop=coordinates|extracts&generator=geosearch&ggscoord=' + lat + '|' + lon + '&ggsradius=5000&ggslimit=1&exchars=550&exintro&explaintext&format=json';
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.status === 200) {
      var res = JSON.parse(req.responseText);
      if (res.query && res.query.pages) {
        var pageId = Object.keys(res.query.pages)[0];
        var page = res.query.pages[pageId];
        
        targetLat = page.coordinates[0].lat;
        targetLon = page.coordinates[0].lon;
        
        var dist = getDistance(lat, lon, targetLat, targetLon);
        var bear = getBearing(lat, lon, targetLat, targetLon);

        Pebble.sendAppMessage({
          'MESSAGE_KEY_TITLE': page.title,
          'MESSAGE_KEY_SNIPPET': page.extract,
          'MESSAGE_KEY_DISTANCE': Math.round(dist),
          'MESSAGE_KEY_BEARING': Math.round(bear)
        });
      } else {
        sendStatus("No results found.");
      }
    }
  };
  req.send(null);
}

function startLocationTracking() {
  if (isTracking) return;
  isTracking = true;

  navigator.geolocation.watchPosition(
    function(pos) {
      var curLat = pos.coords.latitude;
      var curLon = pos.coords.longitude;

      if (lastFetchLat === 0) {
        // Erster Fix
        fetchWiki(curLat, curLon);
      } else {
        var distSinceLastFetch = getDistance(curLat, curLon, lastFetchLat, lastFetchLon);
        
        if (distSinceLastFetch > FETCH_THRESHOLD) {

          fetchWiki(curLat, curLon);
        } else if (targetLat !== 0) {

          var distToTarget = getDistance(curLat, curLon, targetLat, targetLon);
          var bearToTarget = getBearing(curLat, curLon, targetLat, targetLon);
          
          Pebble.sendAppMessage({
            'MESSAGE_KEY_DISTANCE': Math.round(distToTarget),
            'MESSAGE_KEY_BEARING': Math.round(bearToTarget)
          });
        }
      }
    },
    function(err) { sendStatus("GPS failed."); },
    { timeout: 15000, enableHighAccuracy: true, maximumAge: 5000 }
  );
}

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.MESSAGE_KEY_READY) {
    if (e.payload.MESSAGE_KEY_LOCALE) {
      currentLocale = e.payload.MESSAGE_KEY_LOCALE.substring(0, 2);
    }
    sendStatus("Fixing GPS...");
    startLocationTracking();
  }
});

Pebble.addEventListener('ready', function() {});