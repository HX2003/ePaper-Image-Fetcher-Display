function doGet(e){
  var ss = SpreadsheetApp.openById("???");
  var sheet = ss.getSheetByName('Sheet1'); // or whatever is the name of the sheet 
  var range = sheet.getRange(1,1); 
  var subreddit = range.getValue();
  
  var reddit = UrlFetchApp.fetch("https://www.reddit.com/r/"+subreddit+"/new.json?limit=1&raw_json=1");
  var redditObject = JSON.parse(reddit);
  var title = redditObject["data"]["children"][0]["data"]["title"];
  var target_height = 800;
  var target_width = 600;
  var image_sizes = 0;
  var response = ""
  var text = ""
  var url = ""
  
  if(redditObject["data"]["children"][0]["data"].hasOwnProperty('selftext')){
    if(redditObject["data"]["children"][0]["data"]["selftext"]!==""){
      //This a text post
      text = redditObject["data"]["children"][0]["data"]["selftext"];
      text = text.substring(0, 3500);
      var newJsonObject = {"title": title, "text": text, "url": url, "scaling_req" : 0}
      
      return ContentService.createTextOutput(JSON.stringify(newJsonObject)); 
    }
  }
  if(redditObject["data"]["children"][0]["data"].hasOwnProperty('preview')){
    image_sizes = redditObject["data"]["children"][0]["data"]["preview"]["images"][0]["resolutions"].length; 
    for (var i = 0; i < image_sizes ; i++) {
      var width = redditObject["data"]["children"][0]["data"]["preview"]["images"][0]["resolutions"][i].width;
      var height = redditObject["data"]["children"][0]["data"]["preview"]["images"][0]["resolutions"][i].height;
      url = redditObject["data"]["children"][0]["data"]["preview"]["images"][0]["resolutions"][i].url;
      if(width>=target_width&&height>=target_height){
        var newJsonObject = {"title": title, "text": text, "url": url, "scaling_req" : 0}
        
        return ContentService.createTextOutput(JSON.stringify(newJsonObject)); 
      }
      if(i==image_sizes-1){
        //Reached the end of images, if image sizes are all too small no choice, this is already the largest image
        var newJsonObject = {"title": title, "text": text, "url": url, "scaling_req" : 1}
        
        return ContentService.createTextOutput(JSON.stringify(newJsonObject)); 
      }
    }
  }
  
  var newJsonObject = {"title": title, "text": "No text or image available", "url": url, "scaling_req" : 0}
  
  return ContentService.createTextOutput(JSON.stringify(newJsonObject)); 
}