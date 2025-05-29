document.addEventListener('DOMContentLoaded', function() {
  // Validate form inputs
  const form = document.querySelector('form');
  form.addEventListener('submit', function(e) {
      let valid = true;
      document.querySelectorAll('.form-control').forEach(input => {
          if (!input.value.trim()) {
              input.style.borderColor = 'red';
              valid = false;
          } else {
              input.style.borderColor = '#ccc';
          }
      });
      if (!valid) {
          e.preventDefault();
          alert('Please fill in all fields.');
      }
  });

  // Color-code the result based on status word
  const resultEl = document.querySelector('.result h3');
  if (resultEl) {
      const text = resultEl.textContent;
      let cls = '';
      if (text.includes('(Good)'))                   cls = 'good';
      else if (text.includes('(Moderate)'))          cls = 'moderate';
      else if (text.includes('Sensitive Groups)'))   cls = 'unhealthy-for-sensitive-groups';
      else if (text.includes('(Unhealthy)'))         cls = 'unhealthy';
      else if (text.includes('(Very Unhealthy)'))    cls = 'very-unhealthy';
      else if (text.includes('(Deadly)'))            cls = 'deadly';
      resultEl.classList.add(cls);
  }
});
