// WikiRadius v2.0.0 - pkjs/index.js

function sendStatus(msg) {
  Pebble.sendAppMessage({'MESSAGE_KEY_STATUS': msg});
}

function fetchWiki(lat, lon) {
  sendStatus("Wikipedia Suche...");
  var url = 'https://de.wikipedia.org/w/api.php?action=query&list=geosearch&gscoord=' + lat + '|' + lon + '&gsradius=5000&gslimit=1&format=json';
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.status === 200) {
      var res = JSON.parse(req.responseText);
      if (res.query.geosearch.length > 0) {
        var item = res.query.geosearch[0];
        fetchSnippet(item.pageid, item.title, item.dist);
      } else { sendStatus("Keine Treffer."); }
    }
  };
  req.send(null);
}

function fetchSnippet(id, title, dist) {
  sendStatus("Lade Text...");
  var url = 'https://de.wikipedia.org/w/api.php?action=query&prop=extracts&exchars=500&exintro&explaintext&pageids=' + id + '&format=json';
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.status === 200) {
      var res = JSON.parse(req.responseText);
      var snippet = res.query.pages[id].extract;
      
      Pebble.sendAppMessage({
        'MESSAGE_KEY_TITLE': title,
        'MESSAGE_KEY_SNIPPET': snippet,
        'MESSAGE_KEY_DISTANCE': Math.round(dist),
        'MESSAGE_KEY_BEARING': Math.floor(Math.random() * 360)
      });
    }
  };
  req.send(null);
}

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.MESSAGE_KEY_READY) {
    sendStatus("GPS Fix...");
    navigator.geolocation.getCurrentPosition(
      function(pos) { fetchWiki(pos.coords.latitude, pos.coords.longitude); },
      function(err) { sendStatus("GPS Fehler."); },
      { timeout: 15000, enableHighAccuracy: true }
    );
  }
});

Pebble.addEventListener('ready', function() { console.log("JS Ready"); });
