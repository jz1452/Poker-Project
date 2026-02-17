/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,jsx}"],
  theme: {
    extend: {
      colors: {
        felt: {
          900: "#0f3c2f",
          800: "#14523f",
          700: "#1a6650"
        }
      },
      boxShadow: {
        table: "0 24px 60px rgba(0,0,0,0.45)"
      }
    }
  },
  plugins: []
};
