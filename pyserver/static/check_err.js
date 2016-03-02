function checkError(json) {
   if(json.status === 'error')
      alert('Error: ' + json.description)
} 
