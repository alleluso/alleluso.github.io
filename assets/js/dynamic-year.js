/**
 * Dynamic Year Script
 * Updates the current year in elements with id="current-year"
 */

document.addEventListener('DOMContentLoaded', function() {
  // Update current year dynamically
  const currentYearElement = document.getElementById('current-year');
  if (currentYearElement) {
    currentYearElement.textContent = new Date().getFullYear();
  }
  
  // You can add more dynamic date functionality here if needed
  // For example, updating copyright years, project dates, etc.
});
